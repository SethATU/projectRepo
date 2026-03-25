#include <esp_now.h>
#include <WiFi.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
//#define MQTT_HOST "broker.emqx.io"
//#define MQTT_PORT 8084
//#define MQTT_PUB_DIST "seth/esp32/distance"
//#define MQTT_PUB_HUMI "seth/esp32/humidity"
//#define MQTT_PUB_CELC "seth/esp32/celcius"
//#define MQTT_PUB_FARA "seth/esp32/farenheight"
//#define MQTT_PUB_LATT "seth/esp32/latitude"
//#define MQTT_PUB_LONG "seth/esp32/longitude"
//#define MQTT_PUB_MOVE "seth/esp32/movement"
//#define MQTT_PUB_ALAR "seth/esp32/alarm"
//#define MQTT_PUB_USER "seth/esp32/user"
//#define MQTT_PUB_KEY "seth/esp32/key"
#define BUZZ 32

typedef struct struct_message1 {
  float dist;
  float humi;
  float celc;
  float fara;
  double latt;
  double lonn;
  int move;
} struct_message1;
struct_message1 incomingReadings1;

typedef struct struct_message2 {
  int alar;
  int user;
  int key;
} struct_message2;
struct_message2 incomingReadings2;

float distance;
float humidity;
float celcius;
float farenheight;
double lattNum;
double longNum;
String moveString = "Error";
String alarmString = "Error";
String userString = "Error";
String keyString = "Error";

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  if(len == sizeof(struct_message1)) {
    memcpy(&incomingReadings1, incomingData, sizeof(incomingReadings1));
  } else if(len == sizeof(struct_message2)) {
    memcpy(&incomingReadings2, incomingData, sizeof(incomingReadings2));
  }

  distance = incomingReadings1.dist;
  humidity = incomingReadings1.humi;
  celcius = incomingReadings1.celc;
  farenheight = incomingReadings1.fara;
  lattNum = incomingReadings1.latt;
  longNum = incomingReadings1.lonn;

  if(incomingReadings1.move == 1) { moveString = "Movement";}
  else { moveString = "No Movement"; }

  if(incomingReadings2.alar == 0) { alarmString = "Disarmed"; }
  else { alarmString = "Active"; }

  if(incomingReadings2.user == 1) { userString = "Seth K"; }
  else if(incomingReadings2.user == 2) { userString = "Seth F"; }
  else { userString = "Unknown"; }

  if(incomingReadings2.key == 1) { keyString = "Card"; }
  else if(incomingReadings2.key == 2) { keyString = "Fob"; }
  else { keyString = "Unknown"; }


  //serial print the data that is sent from modual 1 and 2
  Serial.printf("------------------------------------------------\n");
  Serial.printf("DISTANCE: %.2fcm\n", incomingReadings1.dist);
  Serial.printf("MOVEMENT: %d\n", incomingReadings1.move);  
  Serial.printf("LATITUDE: %.6f\n", incomingReadings1.latt);
  Serial.printf("LONGITUDE: %.6f\n", incomingReadings1.lonn);
  Serial.printf("CELCIUS: %.2f°C\n", incomingReadings1.celc);
  Serial.printf("FARENHEIGHT: %.2f°F\n", incomingReadings1.fara);
  Serial.printf("HUMIDITY: %.2f%%\n", incomingReadings1.humi);
  Serial.printf("------------------------------------------------\n");
  Serial.printf("ALARM: %d\n", incomingReadings2.alar);
  Serial.printf("USER: %d\n", incomingReadings2.user);
  Serial.printf("KEY: %d\n", incomingReadings2.key);
  Serial.printf("------------------------------------------------\n");
}

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("Connected to Wi-Fi");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  connectToWifi();       

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  else {
    Serial.println("ESP-NOW Initalised");
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  pinMode(BUZZ, OUTPUT);
}

void loop() {
  if(incomingReadings1.move == 1 && incomingReadings2.alar == 1) {
    alarmString = "Alert";
    digitalWrite(BUZZ, HIGH);
    delay(250);
    digitalWrite(BUZZ, LOW);
    delay(250);
  }
}