#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "Adafruit_SGP30.h"
#include "Adafruit_SHT31.h"

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>

#define WIFI_SSID     "INSERT THINGS HERE"
#define WIFI_PASSWORD "INSERT THINGS HERE"
#define MQTT_SERVER   "INSERT THINGS HERE"
#define MQTT_PORT     1883

ESP8266WebServer server(80);
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_SGP30 sgp;

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT);
Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, "prometheus/room/bedroom/temperature");
Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, "prometheus/room/bedroom/humidity");
Adafruit_MQTT_Publish eco2Feed = Adafruit_MQTT_Publish(&mqtt, "prometheus/room/bedroom/eCO2");
Adafruit_MQTT_Publish tvocFeed = Adafruit_MQTT_Publish(&mqtt, "prometheus/room/bedroom/tvoc");


void handle_root() {
    server.send(200, "text/plain", "Hello from the weather esp8266, read from /temp or /humidity");
    delay(100);
}


/* return absolute humidity [mg/m^3] with approximation formula
 * @param temperature [°C]
 * @param humidity [%RH]
 */
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

void setup() {
    Serial.begin(115200);

    // Connect to wifi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("\n\r \n\rWorking to connect");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WIFI_SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    // Start sensors
    if (! sht31.begin(0x44)) {
        Serial.println("Temperature/Humidity (SHT31) sensor not found");
        while (1);
    }
    if (! sgp.begin()){
        Serial.println("Air Quality (SGP30) sensor not found");
        while (1);
    }


    // Start wifi server
    server.on("/", handle_root);
    server.on("/stats", [](){
        String info = get_stats();
        server.send(200, "text/plain", info);
    });
    server.begin();
}


String get_stats() {
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();
    sgp.setHumidity(getAbsoluteHumidity(t, h));
    sgp.IAQmeasure();
    int tvoc = sgp.TVOC;
    int co2 = sgp.eCO2;
    return "Temp: " + String((int)t) + "\nHumidity: "+ String((int)h) + "\ntVOC: " + String(tvoc) + "\neCO2: " + String(co2) + "\n";
}

int period = 2000;
int tick = 0;

void loop() {
    int now = millis();
    server.handleClient();

    // Only update once every period
    if (now - tick > period) {
        tick = now;
        MQTT_connect();
        float t = sht31.readTemperature();
        float h = sht31.readHumidity();
        sgp.setHumidity(getAbsoluteHumidity(t, h));
        sgp.IAQmeasure();
        int tvoc = sgp.TVOC;
        int co2 = sgp.eCO2;

        // Subtract 8 degrees, calibrated 2019-09-15
        float temp_f = (1.8 * t) + 32.0 - 8.0;
        Serial.print("Publishing temp: "); Serial.println(temp_f);
        temperatureFeed.publish(temp_f);

        Serial.print("Publishing humidity: "); Serial.println(h);
        humidityFeed.publish(h);

        Serial.print("Publishing co2: "); Serial.println(co2);
        eco2Feed.publish(co2);

        Serial.print("Publishing tvoc: "); Serial.println(tvoc);
        tvocFeed.publish(tvoc);
    }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
