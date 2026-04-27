#include <esp_now.h>  
#include <esp_wifi.h> 
#include <WiFi.h>
#include <Wire.h>   
#include "rgb_lcd.h"
#include <Keypad.h>
#include <SPI.h>    
#include <MFRC522.h>

#define ROW 4         //keypad rows
#define COLUMN 3      //keypad columns 
#define SS_RFID 21    //rfid slave pin
#define RST_RFID 22   //rfid reset pin
#define SCK_RFID 25   //rfid clock pin
#define MOSI_RFID 26  //rfid master out slave in
#define MISO_RFID 27  //rfid slave out master in 

rgb_lcd lcd;

//keypad layout            
char keys[ROW][COLUMN] =  {    
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
//keypad gpio pins 
byte pin_rows[ROW] = {2, 0, 4, 16};   
byte pin_column[COLUMN] = {17, 5, 18};
//keypad object
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW, COLUMN);
byte unlockCard[4] = {0x65, 0x74, 0x4D, 0x05}; //UID bytes for key card
byte unlockFob[4] = {0x26, 0xF4, 0xAF, 0x01};  //UID bytes for key fob 
byte newHex[4]; //buffer to save read card UID
MFRC522 rfid(SS_RFID, RST_RFID); 

int code[4] = {1, 2, 3, 4}; //keypad code 
int check[4];
bool prompt = true;
int scan = 0;
int incorrect = 0;
int alarmState = 0;
int codeIndex = 0;
int atempt = 3;
int end = 0;
bool allowSend = false;
bool fobMatch = true;
bool cardMatch = true;

//esp-main/ recever MAC adress
uint8_t broadcastAddress[] = {0x84, 0x0D, 0x8E, 0xE6, 0x8F, 0xB4};

//Structure of data packets to be sent with espnow
typedef struct struct_message2 {
  int alar;
  int user;
  int key;
} struct_message2;

esp_now_peer_info_t peerInfo; //recever connectin info
struct_message2 myData; //data that is being sent 

//wifi channel setup 
//espnow needs both esp32s to be on same wifi channel 
constexpr char WIFI_SSID[] = "Backup"; 
int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

//espnow call back, checks if data was sent and displays if it sent or not 
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); //set wifi as client 
  WiFi.disconnect();  //disconnect from all pasted wifi to make sure on same channel 
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE); //set defult channel 
  Wire.begin(19, 23); //lcd Pin setup
  lcd.begin(16, 2); //lcd display width 
  SPI.begin(SCK_RFID, MISO_RFID, MOSI_RFID, SS_RFID); //rfid pin setup

  rfid.PCD_Init(); //initalise MRFC522

  int32_t channel = getWiFiChannel(WIFI_SSID); //find wifi channel so esp now uses same one 

  WiFi.printDiag(Serial); //wifi disagnostics
  //espnow channel setup to connect to correct channel 
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); 

  //esp now init, return ok if sucsessful connect 
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  //esp send call back and delivery status 
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  //espnow encription, set to false 
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;

  //register peer device 
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  //lcd startup loading bar animation
  lcd.print("Loading"); 
  for (int l = 0; l < 9; l++) {
    lcd.print(".");
    delay(250);
  }
  lcd.clear();
}

void loop() {
  //rfid timeout 
  if(end == 1) {
    lcd.print("Timed Out");
    delay(1000);
    lcd.clear();
    end = 0;
  }
  keypadRead(); //keypad function

  //if security process worked then allow send of espnow data 
  if(allowSend) {
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
  allowSend = false; //reset send after data sent 
}

void keypadRead() {
  char key = keypad.getKey();

  //alarm state / active or inactive
  if(codeIndex == 0 && prompt) {
    if(alarmState == 0) { lcd.print("Alarm Disabled"); }
    else { lcd.print("Alarm Active"); }
    lcd.setCursor(0, 1);
    lcd.print("Enter Code: ");  
    prompt = false;
  }

  //user enters 4 keys on keypad 
  if(key) {  
    if(key >= '0' && key <= '9') {
      lcd.print(key);
      check[codeIndex] = key - '0'; //conver char to int
      codeIndex++;
    }
    if(codeIndex == 4) { //user has entered 4 keys 
      codeIndex = 0;
      prompt = true;
      delay(1000);
      codeCheck(); // check code entered is correct 
    }
  }
}

void codeCheck() {
  incorrect = 0;
  lcd.clear();

  //compare code entered to saved unlock code 
  for(int j = 0; j < 4; j++) {
    if(check[j] != code[j]) {
      incorrect++;
    }
  }
  if(incorrect > 0 && alarmState == 1) { atempt--; }
  //code correct, rfid 2fa 
  if(incorrect == 0) {
    atempt = 3;
    rfidRead();
    return;
  }
  else if(incorrect > 0 && alarmState == 0) { //code incorrect and alarm not active
    lcd.print("Incorrect");
    lcd.setCursor(0, 1);
    lcd.print("Try Again");
    delay(2000);
  }
  else if(incorrect > 0 && alarmState == 1 && atempt > 0) { //code incorrect, alarm active, and more than 3 attempts 
    lcd.print("Incorrect");
    lcd.setCursor(0, 1);
    lcd.print("Attempts Left: ");
    lcd.print(atempt);
    delay(2000);
  }
  else { //alarm active and out of attempts, sound alarm 
    lcd.print("Incorrect");
    lcd.setCursor(0, 1);
    lcd.print("Sounding Alarm");
    delay(2000);
  }
  lcd.clear();
}

void rfidRead() { //rfid 2fa 
  scan = 0;
  cardMatch = true;
  fobMatch = true;
  lcd.print("2FA / Scan Card:");
  lcd.setCursor(0, 1);

  unsigned long start = millis(); //start time 
  const unsigned long timeout = 10000; //10 sec timer 
  
  //wait loop while scannic card or run out of time 
  while(true) {
    if(!rfid.PICC_IsNewCardPresent() && !rfid.PICC_ReadCardSerial()) { 
      if(millis() - start >= timeout) {
        end = 1; //display in loop that card timed out 
        return; //exit rfid read function
      }
    } 
    if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) { 
      break; //break whait loop 
    }
  }
  //save new card scanned to the buffer 
  for(byte i = 0; i < rfid.uid.size; i++) {
    newHex[i] = rfid.uid.uidByte[i];
  }

  //compare the UID to see if its the users card or fob 
  for(byte j = 0; j < rfid.uid.size; j++) {
    if(rfid.uid.uidByte[j] != unlockCard[j]) {
      cardMatch = false;
    }
    if(rfid.uid.uidByte[j] != unlockFob[j]) {
      fobMatch = false;
    }
  }
  //set scan results 
  if(cardMatch) {scan = 1;}
  if(fobMatch) {scan = 2;}
  //set espnow data to be sent 
  if(scan == 1) { 
    lcd.print("Valid Card");
    if(alarmState == 1) { alarmState = 0; }
    else { alarmState = 1; }
    myData.alar = alarmState;
    myData.user = 1;
    myData.key = 1;
    allowSend = true;
    } 
  else if(scan == 2) {
    lcd.print("Valid Fob");
    if(alarmState == 1) { alarmState = 0; }
    else { alarmState = 1; }
    myData.alar = alarmState;
    myData.user = 2;
    myData.key = 2;
    allowSend = true;
  }
  else { 
    lcd.print("Not valid");
    myData.alar = alarmState;
    myData.user = 0;
    myData.key = 0;
    allowSend = true;
  }

  delay(1000);
  lcd.clear();
  codeIndex = 0;
  prompt = true; 
}