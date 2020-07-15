## SimpleWeather
Connects a DHT11 or DHT22 to an ESP-8266, then sends temperature and humidity over MQTT.

I use an ESP-01 to keep things cheap. The goal is to have one of these things per room and one outside, so low cost is a high priority.

Keep the ESP separated from the DHT &mdash; or else the heat from the wifi module affects the temperature and humidity readings.

### Setup
Flash your device and it will create a wifi network called SimpleWeather. Connect to this and wait for the captive portal, or visit 192.168.4.1 to set up. Fields are:
1. Wifi network to join
2. Wifi password
3. MQTT server address
4. MQTT server port
5. Temperature topic
6. Humidity topic

If your ESP device needs clearing out (old wifi credentials, etc) then set `resetWifiSettings` to true. Flash and let the device restart, then set it back to false and reflash again. Re-run setup.