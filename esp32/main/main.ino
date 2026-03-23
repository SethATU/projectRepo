#include <esp_now.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}
#include <AsyncMqttClient.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define MQTT_HOST IPAddress()
#define MQTT_PORT 1883
#define MQTT_PUB_DIST "esp32/distance"
#define MQTT_PUB_HUMI "esp32/humidity"
#define MQTT_PUB_CELC "esp32/celcius"
#define MQTT_PUB_FARA "esp32/farenheight"
#define MQTT_PUB_LATT "esp32/latitude"
#define MQTT_PUB_LONG "esp32/longitude"
#define MQTT_PUB_MOVE "esp32/movement"
#define MQTT_PUB_ALAR "esp32/alarm"
#define MQTT_PUB_USER "esp32/user"
#define MQTT_PUB_KEY "esp32/key"
#define BUZZ 32

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

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

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
  espSendMqtt();
}

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); 
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  connectToWifi();       

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectToWifi();

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

void espSendMqtt() {
  uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_DIST, 1, true, String(distance).c_str());
  uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUMI, 1, true, String(humidity).c_str());
  uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_CELC, 1, true, String(celcius).c_str());
  uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_FARA, 1, true, String(farenheight).c_str());
  uint16_t packetIdPub5 = mqttClient.publish(MQTT_PUB_LATT, 1, true, String(lattNum, 6).c_str());
  uint16_t packetIdPub6 = mqttClient.publish(MQTT_PUB_LONG, 1, true, String(longNum, 6).c_str());
  uint16_t packetIdPub7 = mqttClient.publish(MQTT_PUB_MOVE, 1, true, moveString.c_str());
  uint16_t packetIdPub8 = mqttClient.publish(MQTT_PUB_ALAR, 1, true, alarmString.c_str());
  uint16_t packetIdPub9 = mqttClient.publish(MQTT_PUB_USER, 1, true, userString.c_str());
  uint16_t packetIdPub10 = mqttClient.publish(MQTT_PUB_KEY, 1, true, keyString.c_str());
}