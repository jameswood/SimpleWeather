#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// #define DHTMODE
#define ONEWIREMODE

#ifdef DHTMODE
  #include <DHT.h>
  DHT dht(2, DHT11);
#endif

#ifdef ONEWIREMODE
  #include <DallasTemperature.h>
  OneWire oneWire(2);
  DallasTemperature sensors(&oneWire);
  DeviceAddress insideThermometer;
#endif

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const bool debug = false;
const char* ssid = "*******";
const char* password = "*******";
const char* hostName = "bedroomTemp";
const char* mqtt_server = "192.168.1.2";
const uint16_t mqtt_port = 1883;
const char* statusTopic = "device/bedroomTemp/status";
const char* humidityTopic = "sensors/bedroom/humidity";
const char* temperatureTopic = "sensors/bedroom/temperature";

const long measurementInterval = 30000;
unsigned long lastMeasureAttempt = 0;

void yield(unsigned long yieldDuration) { //overload yield function to allow for any ms delay
  unsigned long yieldBegan = millis();
  while ( (millis() - yieldBegan) < yieldDuration ) yield();
}

void reportResults(float temp, float humid) {
  if (!isnan(temp) || !isnan(humid)) {
    if(debug) {
      Serial.println("-----------------------");
      Serial.println("Humidity (%): " + String(humid));
      Serial.println("Temperature (°C): " + String(temp));
      Serial.println("-----------------------");
    }
    mqttClient.publish(humidityTopic, String(humid).c_str(), false);
    mqttClient.publish(temperatureTopic, String(temp).c_str(), false);
  }
}

void reportResults(float temp) {
  if (!isnan(temp)) {
    if(debug) {
      Serial.println("-----------------------");
      Serial.println("Temperature (°C): " + String(temp));
      Serial.println("-----------------------");
    }
    mqttClient.publish(temperatureTopic, String(temp).c_str(), false);
  }
}

void reconnectMqtt() {
  while (!mqttClient.connected()) {
    if(debug) Serial.print("Attempting MQTT connection...");
    // boolean connect (clientID, [username, password], [willTopic, willQoS, willRetain, willMessage], [cleanSession])
    if (mqttClient.connect(hostName, NULL, NULL, statusTopic, 1, true, "offline")) {
      if(debug) Serial.println("connected");
      mqttClient.publish(statusTopic, "online", true);
    } else {
      if(debug) {
        Serial.print(" failed. MQTT connection error: ");
        Serial.println(mqttClient.state());
        Serial.println(" try again in 5 seconds");
      }
      yield(5000);
    }
  }
}

void setup_wifi() {
  delay(10);
  if(debug){
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if(debug){
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    } 
    Serial.print("\nWiFi connected!\nIP address: ");
    Serial.println(WiFi.localIP());
  }
}

void setup() {
  if(debug) Serial.begin(115200);
  #ifdef ONEWIREMODE
    sensors.begin();
    if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
    sensors.setResolution(insideThermometer, 12);
  #endif
  #ifdef DHTMODE
    dht.begin();
  #endif
  setup_wifi();
  mqttClient.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!mqttClient.connected()) reconnectMqtt();
  mqttClient.loop();
  if ((millis() - lastMeasureAttempt) > measurementInterval) {
    #ifdef DHTMODE
      reportResults(dht.readTemperature(), dht.readHumidity());
    #endif
    #ifdef ONEWIREMODE
      sensors.requestTemperatures();
      reportResults(sensors.getTempC(insideThermometer));
    #endif
    lastMeasureAttempt = millis();
  }
}
