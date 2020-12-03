## SimpleWeather
Connects a DHT11 or DHT22 to an ESP-8266, then sends temperature and humidity over MQTT.

I use an ESP-01 to keep things cheap. The goal is to have one of these things per room and one outside, so low cost is a high priority.

Keep the ESP separated from the DHT &mdash; or else the heat from the wifi module affects the temperature and humidity readings.

### Setup
Edit your wifi/mqtt settings and flash your device.