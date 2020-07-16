#include <LittleFS.h>             //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11

const bool resetWifiSettings = false; // set true to clear out old settings and start fresh!
bool shouldSaveConfig = false;
const bool debug = false;

DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

char hostName[40] = "SimpleWeather";
char mqtt_server[40];
char mqtt_port[6];
char humidityTopic[40];
char temperatureTopic[40];

const long measurementInterval = 5000;
unsigned long lastMeasureAttempt = 0;

void yield(unsigned long yieldDuration) { //overload yield function to allow for any ms delay
  unsigned long yieldBegan = millis();
  while ( (millis() - yieldBegan) < yieldDuration ) yield();
}

void saveConfigCallback () {
  if(debug) Serial.println("Should save config");
  shouldSaveConfig = true;
}

void reportResults(float temp, float humid) {
  if (!isnan(temp) || !isnan(humid)) {
    if(debug) {
      Serial.println("-----------------------");
      Serial.println("Humidity (%): " + String(humid));
      Serial.println("Temperature (°C): " + String(temp));
      Serial.println("-----------------------");
    }
    mqttClient.publish(humidityTopic, String(humid).c_str(), true);
    mqttClient.publish(temperatureTopic, String(temp).c_str(), true);
  }
}

void reconnectMqtt() {
  while (!mqttClient.connected()) {
    if(debug) Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(hostName)) {
      if(debug) Serial.println("connected");
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

void setup() {
  if(debug) Serial.begin(115200);
  if(resetWifiSettings) LittleFS.format();
  if(debug) Serial.println("mounting FS...");

  if (LittleFS.begin()) {
    if(debug) Serial.println("mounted file system");
    if (LittleFS.exists("/config.json")) {
      //file exists, reading and loading
      if(debug) Serial.println("reading config file");
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        if(debug) Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        if(debug) json.printTo(Serial);
        if (json.success()) {
          if(debug) Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(hostName, json["hostName"]);
          strcpy(humidityTopic, json["humidityTopic"]);
          strcpy(temperatureTopic, json["temperatureTopic"]);
        } else {
          if(debug) Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    if(debug) Serial.println("failed to mount FS");
  }
  //end read

  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_hostname("hostname", "host name", hostName, 40);
  WiFiManagerParameter custom_mqtt_humidityTopic("humidityTopic", "humidity topic", humidityTopic, 40);
  WiFiManagerParameter custom_mqtt_temperatureTopic("temperatureTopic", "temperature topic", temperatureTopic, 40);

  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_hostname);
  wifiManager.addParameter(&custom_mqtt_humidityTopic);
  wifiManager.addParameter(&custom_mqtt_temperatureTopic);

  if (resetWifiSettings) wifiManager.resetSettings();
  if (!wifiManager.autoConnect("SimpleWeather", "password")) {
    if(debug) Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  if(debug) Serial.println("connected!");

  //read updated parameters
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(hostName, custom_mqtt_hostname.getValue());
  strcpy(humidityTopic, custom_mqtt_humidityTopic.getValue());
  strcpy(temperatureTopic, custom_mqtt_temperatureTopic.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    if(debug) Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["hostName"] = hostName;
    json["humidityTopic"] = humidityTopic;
    json["temperatureTopic"] = temperatureTopic;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) Serial.println("failed to open config file for writing");

    if(debug) json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  if(debug) Serial.println("local ip");
  if(debug) Serial.println(WiFi.localIP());
  uint16_t mqtt_port_int = (uint16_t)strtol(mqtt_port, NULL, 10);
  mqttClient.setServer(mqtt_server, mqtt_port_int);
  dht.begin();
}

void loop() {
  if (!mqttClient.connected()) reconnectMqtt();
  mqttClient.loop();
  if ((millis() - lastMeasureAttempt) > measurementInterval) {
    reportResults(dht.readTemperature(), dht.readHumidity());
    lastMeasureAttempt = millis();
  }
}
