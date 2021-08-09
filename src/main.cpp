#include <Arduino.h>
#include <secrets.h>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #define SENSOR_apin 13
#endif
#ifdef ESP32
  #include <WiFi.h>
  #define SENSOR_apin 36
#endif

#include <PubSubClient.h>
#include <ArduinoJson.h>
// #define OFFSET 1.4 (if needed to calibrate sensor)
#define MQTT_CLIENT_ID  "Soil_Sensor"
#define MQTT_SENSOR_TOPIC "soil/sensor1"
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  300
#define SENSITIVITY (3.3 / 1024.0)

const char* ssid = WIFI_SSID;
const char* password = WIFI_PW;


const char broker[] = BROKER;
uint16_t port = 1883;


long lastMsg = 0;

float humidity;
float batteryPct;


WiFiClient espClient;
PubSubClient client(espClient);

//function declarations
void setup_wifi();
void callback(char* topic, byte* message, unsigned int length);
void reconnect();
void print_wakeup_reason();
float getBatteryPercentage();
float mapf(float x, float in_min, float in_max, float out_min, float out_max);

void setup() {

  Serial.begin(9600);
  while(!Serial){
    // wait for serial port to connect
  }
#ifdef esp32
  print_wakeup_reason();
#endif
  //Initialiaze sensor
  delay(500);//Delay to let system boot
  Serial.println("Soil Humidity Sensor\n\n");
  delay(1000);//Wait before accessing Sensor

  //Initialize WIFI and wait till connected
  
  setup_wifi();
  client.setServer(broker,port);
  //client.setCallback(callback);

}

void loop() {
  if (!client.connected()){
    reconnect();
  }
  client.loop();

  for (int i = 0; i<=100; i++){
    humidity += analogRead(SENSOR_apin);
    delay(1);
  }

  humidity = humidity / 100;
  Serial.print("Current soil humidity = ");
  Serial.print(humidity);
  Serial.println();
  int pctHumidity = map(humidity, 0,950,0,100);
  Serial.print("Humidity percentage: ");
  Serial.println(pctHumidity);
  Serial.println();

  batteryPct = getBatteryPercentage();

  StaticJsonDocument<300> doc;
  doc["device"] = "Soil_Sensor";
  doc["humidity"] = humidity;
  doc["BatteryPct"] = batteryPct;

  char buffer[256];
  serializeJsonPretty(doc, Serial);
  Serial.println();

  serializeJson(doc, buffer);

  client.publish(MQTT_SENSOR_TOPIC, buffer);

  #ifdef esp32
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP*uS_TO_S_FACTOR);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  #endif

  Serial.println("Sleep ...");
  Serial.flush();
  #ifdef ESP32
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP*uS_TO_S_FACTOR);
  #endif
  #ifdef ESP8266
    ESP.deepSleep(TIME_TO_SLEEP*uS_TO_S_FACTOR);
  #endif
}

void setup_wifi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.println("Connecting to WiFi..");
}
  Serial.println("You're connected to the network");
  Serial.println();
}

void callback(char* topic, byte* message, unsigned int length){
//can be added to read incoming messages
}

void reconnect(){
    while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      // client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

float getBatteryPercentage(){
    float batteryValue = analogRead(A0);
    Serial.print("Raw battery value: ");
    Serial.println(int(batteryValue));

    batteryValue = batteryValue * SENSITIVITY;
    Serial.print("Voltage: ");
    Serial.print(batteryValue);
    Serial.println("V");

    batteryValue = mapf(batteryValue, 1.98, 3.09, 0, 100);
    Serial.print("Battery percentage: ");
    Serial.print(int(batteryValue));
    Serial.println("%");

    return batteryValue;

}

float mapf(float x, float in_min, float in_max, float out_min, float out_max){
  float a = x - in_min;
  float b = out_max - out_min;
  float c = in_max - in_min;
  return a * b / c + out_min;
}
#ifdef esp32
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
#endif


