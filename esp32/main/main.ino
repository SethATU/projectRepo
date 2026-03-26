#include <esp_now.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define WIFI_SSID "Backup"
#define WIFI_PASSWORD "nonono12345"
#define BUZZ 32
#define MQTT_HOST "broker.emqx.io"
#define MQTT_PORT 1883
#define MQTT_USERNAME "emqx"
#define MQTT_PASSWORD "public"

#define MQTT_PUB_DIST "seth/esp32/distance"
#define MQTT_PUB_HUMI "seth/esp32/humidity"
#define MQTT_PUB_CELC "seth/esp32/celcius"
#define MQTT_PUB_FARA "seth/esp32/farenheight"
#define MQTT_PUB_LATT "seth/esp32/latitude"
#define MQTT_PUB_LONG "seth/esp32/longitude"
#define MQTT_PUB_MOVE "seth/esp32/movement"
#define MQTT_PUB_ALAR "seth/esp32/alarm"
#define MQTT_PUB_USER "seth/esp32/user"
#define MQTT_PUB_KEY "seth/esp32/key"

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

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  if(len == sizeof(struct_message1)) {
    memcpy(&incomingReadings1, incomingData, sizeof(incomingReadings1));
  } else if(len == sizeof(struct_message2)) {
    memcpy(&incomingReadings2, incomingData, sizeof(incomingReadings2));
  }

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


  mqtt_client.publish(MQTT_PUB_DIST, String(incomingReadings1.dist).c_str());
  mqtt_client.publish(MQTT_PUB_HUMI, String(incomingReadings1.humi).c_str());
  mqtt_client.publish(MQTT_PUB_CELC, String(incomingReadings1.celc).c_str());
  mqtt_client.publish(MQTT_PUB_FARA, String(incomingReadings1.fara).c_str());
  mqtt_client.publish(MQTT_PUB_LATT, String(incomingReadings1.latt).c_str());
  mqtt_client.publish(MQTT_PUB_LONG, String(incomingReadings1.lonn).c_str());
  mqtt_client.publish(MQTT_PUB_MOVE, String(incomingReadings1.move).c_str());
  mqtt_client.publish(MQTT_PUB_ALAR, String(incomingReadings2.alar).c_str());
  mqtt_client.publish(MQTT_PUB_USER, String(incomingReadings2.user).c_str());
  mqtt_client.publish(MQTT_PUB_KEY, String(incomingReadings2.key).c_str());
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  connectToWiFi();   
  mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
  mqtt_client.setKeepAlive(60);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTT();    

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
  mqtt_client.loop();
  
  if(incomingReadings1.move == 1 && incomingReadings2.alar == 1) {
    alarmString = "Alert";
    digitalWrite(BUZZ, HIGH);
    delay(250);
    digitalWrite(BUZZ, LOW);
    delay(250);
  }
}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void connectToMQTT() {
  while (!mqtt_client.connected()) {
    String client_id = "Esp32";
    Serial.printf("Connecting to MQTT Broker\n");
    if (mqtt_client.connect(client_id.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("Connected to MQTT broker");
      return;
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqttCallback(char *mqtt_topic, byte *payload, unsigned int length) {
  Serial.print("Message Receved: ");
  Serial.println(mqtt_topic);
}