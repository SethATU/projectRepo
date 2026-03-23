#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <DHT.h>
#include <Adafruit_GPS.h>
#include <HardwareSerial.h>
#include "secrets.h"
#include "ThingSpeak.h"

#define DHT11_PIN 2
#define GPSSerial Serial2
Adafruit_GPS GPS(&GPSSerial);
#define GPSECHO false

DHT dht11(DHT11_PIN, DHT11);

uint32_t timer = millis();
const int TRIG = 14;
const int ECHO = 34;
long duration;          
float distance;    
unsigned long previousMillis = 0;
const long interval = 20000;  
unsigned long previousThingspeakMillis = 0;
const long thingspeakInterval = 20000;
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int keyIndex = 0;         
WiFiClient  client;

uint8_t broadcastAddress[] = {0x84, 0x0D, 0x8E, 0xE6, 0x8F, 0xB4};

typedef struct struct_message1 {
  float dist;
  float humi;
  float celc;
  float fara;
  double latt;
  double lonn;
  int move;
} struct_message1;

esp_now_peer_info_t peerInfo;

struct_message1 myData;

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

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
  WiFi.disconnect();
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
  dht11.begin();

  Serial2.begin(9600, SERIAL_8N1, 17, 16);
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); 
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  GPSSerial.println(PMTK_Q_RELEASE);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); 
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); 

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT); 
}

void loop() {
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }

  GPS.read();

  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) 
      return; 
  }

  if (millis() - timer > 2000) {
    timer = millis(); 

    if (GPS.fix) {
      myData.latt = GPS.latitudeDegrees;
      myData.lonn = GPS.longitudeDegrees;

      //remove later
      Serial.println("Location: ");
      Serial.print(GPS.latitudeDegrees, 6);
      Serial.print(", ");
      Serial.print(GPS.longitudeDegrees, 6);
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    distanceRead();
    Serial.println("Distance: " + String(distance) + "CM");
    tempHumRead();
    
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }

  if (currentMillis - previousThingspeakMillis >= thingspeakInterval) {
    previousThingspeakMillis = currentMillis;
    
    ThingSpeak.setField(1, myData.dist);
    ThingSpeak.setField(2, myData.celc);
    ThingSpeak.setField(3, myData.fara);
    ThingSpeak.setField(4, myData.humi);

    int response = ThingSpeak.writeFields(SECRET_CH_ID, SECRET_WRITE_APIKEY);
    if(response == 200){
        Serial.println("ThingSpeak update successful.");
    }else{
        Serial.println("Problem updating ThingSpeak. HTTP error code " + String(response));
    }
  }
}

void distanceRead() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  duration = pulseIn(ECHO, HIGH, 20000); 
  distance = (duration * 0.0343) / 2; 

  myData.dist = round(distance);
  
  if(distance < 20){
    myData.move = 1;
  }
  else {
    myData.move = 0;
  }
}

void tempHumRead() {
  float humidity = dht11.readHumidity();
  float temperature_C = dht11.readTemperature();
  float temperature_F = dht11.readTemperature(true);

  if(isnan(temperature_C) || isnan(temperature_F) || isnan(humidity)) {
    Serial.println("Failed to read DHT11");
  }
  else {
    myData.humi = round(humidity);
    myData.celc = round(temperature_C);
    myData.fara = round(temperature_F);
  }
}