# Room Climate ESP8266

Monitor the temperature, humidity, eCO2, and total VOC. The monitor will both publish to MQTT and a basic webserver.

The MQTT feeds are namespaced under `prometheus` to get picked up by [`mqttgateway`](https://github.com/inuits/mqttgateway). Combine this with prometheus and grafana to get a decent visualization setup.


# Hardware
1. [Adafruit Feather Huzzah (ESP8266)](https://www.adafruit.com/product/2821)
2. [Adafruit SHT31-D Temperature/Humidity Sensor](https://www.adafruit.com/product/2857)
3. [Adafruit SGP30 eCO2/VOC Sensor](https://www.adafruit.com/product/3709)

- Hook up the SDL/SDA (I2c) pins on the sensors to the matching pins on the feather.
- The sensors can receive VIN from the 3.3v pin on the feather.


# Software
- Install [`arduino-cli`](https://github.com/arduino/arduino-cli).
- Verify that your feather shows up on `/dev/ttyUSB0` or change it in the makefile.
- Set your WiFi SSID/password in `room-climate.ino` near the top.
- `make install-board` / `make install-libs` will install the dependencies to build this.
- `make` will compile and upload the sketch to your feather.
- `make serial` will connect to the serial output with `screen`.
