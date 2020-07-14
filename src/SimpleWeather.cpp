#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "math.h"
#include "secrets.h" // modify 'include/secrets.h SAMPLE' with your details and save as 'include/secrets.h'

#define DHTPIN 2
#define DHTTYPE DHT11

const char* hostName = "Humidobot-2";
const char* humidityTopic = "sensors/dht2/humidity";
const char* temperatureTopic = "sensors/dht2/temperature";

unsigned long lastMeasureAttempt = 0;

// DHT dht;
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

const long measurementInterval = 5000; //dht.getMinimumSamplingPeriod();

void setup() {
  Serial.begin(115200);
  Serial.println();
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  dht.begin();
}

void setup_wifi() {
  Serial.print("Connecting to " + String(wifi_ssid));
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    yield(500);
    Serial.print(".");
  }
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(hostName)) {
      Serial.println("connected");
    } else {
      Serial.print(" failed. MQTT connection error: ");
      Serial.println(client.state());
      Serial.println(" try again in 5 seconds");
      yield(5000);
    }
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) setup_wifi();
  if (!client.connected()) reconnect();
  client.loop();
  if ((millis() - lastMeasureAttempt) > measurementInterval) {
    reportResults(dht.readTemperature(), dht.readHumidity());
    lastMeasureAttempt = millis();
  }
}

void reportResults(float temp, float humid) {
  if (!isnan(temp) || !isnan(humid)) {
    Serial.println("-----------------------");
    Serial.println("Humidity (%): " + String(humid));
    Serial.println("Temperature (°C): " + String(temp));
    Serial.println("-----------------------");
    client.publish(humidityTopic, String(humid).c_str(), true);
    client.publish(temperatureTopic, String(temp).c_str(), true);
  }
}

void yield(unsigned long yieldDuration) { //overload yield function to allow for any ms delay
  unsigned long yieldBegan = millis();
  while ( (millis() - yieldBegan) < yieldDuration ) {
    yield();
  }
}
