/*
 * temperature_sensor.ino
 *
 * This example shows how to:
 * 1. define a temperature sensor accessory and its characteristics (in my_accessory.c).
 * 2. report the sensor value to HomeKit (just random value here, you need to change it to your real sensor value).
 *
 *  Created on: 2020-05-15
 *      Author: Mixiaoxiao (Wang Bin)
 *
 * Note:
 *
 * You are recommended to read the Apple's HAP doc before using this library.
 * https://developer.apple.com/support/homekit-accessory-protocol/
 *
 * This HomeKit library is mostly written in C,
 * you can define your accessory/service/characteristic in a .c file,
 * since the library provides convenient Macro (C only, CPP can not compile) to do this.
 * But it is possible to do this in .cpp or .ino (just not so conveniently), do it yourself if you like.
 * Check out homekit/characteristics.h and use the Macro provided to define your accessory.
 *
 * Generally, the Arduino libraries (e.g. sensors, ws2812) are written in cpp,
 * you can include and use them in a .ino or a .cpp file (but can NOT in .c).
 * A .ino is a .cpp indeed.
 *
 * You can define some variables in a .c file, e.g. int my_value = 1;,
 * and you can access this variable in a .ino or a .cpp by writing extern "C" int my_value;.
 *
 * So, if you want use this HomeKit library and other Arduino Libraries together,
 * 1. define your HomeKit accessory/service/characteristic in a .c file
 * 2. in your .ino, include some Arduino Libraries and you can use them normally
 *                  write extern "C" homekit_characteristic_t xxxx; to access the characteristic defined in your .c file
 *                  write your logic code (eg. read sensors) and
 *                  report your data by writing your_characteristic.value.xxxx_value = some_data; homekit_characteristic_notify(..., ...)
 * done.
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include <InfluxDbClient.h>
#include "wifi_info.h"
#include "Adafruit_SGP30.h"
#include "Adafruit_SHT31.h"

#define INFLUXDB_URL "http://influxdb.nullroute.host:8086"
// InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "2cpGewEVxv-o1-kW2HT1dHMkTbaGbvr5RQvtv6sdJSfQ-BvnHPNvrKU7eJdl-_MBLwgne5196hR69n75cnmqvQ=="
// InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
#define INFLUXDB_ORG "7e9bfbc3923ca88e"
// InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
#define INFLUXDB_BUCKET "measurements"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_SGP30 sgp;

Point influxData("measurements");

void setup() {
	Serial.begin(115200);
	wifi_connect(); // in wifi_info.h
	my_homekit_setup();

	// Start sensors
	if (! sht31.begin(0x44)) {
		Serial.println("Temperature/Humidity (SHT31) sensor not found");
		while (1);
	}
	if (! sgp.begin()){
		Serial.println("Air Quality (SGP30) sensor not found");
		while (1);
	}

	influxData.addTag("location", "garage");
	// Check server connection
	if (client.validateConnection()) {
		Serial.print("Connected to InfluxDB: ");
		Serial.println(client.getServerUrl());
	} else {
		Serial.print("InfluxDB connection failed: ");
		Serial.println(client.getLastErrorMessage());
	}
}

void loop() {
	my_homekit_loop();
	delay(10);
}

//==============================
// Homekit setup and loop
//==============================

// access your homekit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_current_temperature;
extern "C" homekit_characteristic_t cha_current_humidity;
extern "C" homekit_characteristic_t cha_tvoc;
extern "C" homekit_characteristic_t cha_co2;

static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;

void my_homekit_setup() {
	arduino_homekit_setup(&config);
}

void influx_report(float temp, float humidity, float co2, float voc) {
	influxData.clearFields();
	influxData.addField("temperature", temp);
	influxData.addField("humidity", humidity);
	influxData.addField("co2", co2);
	influxData.addField("voc", voc);
	if (!client.writePoint(influxData)) {
		Serial.print("InfluxDB write failed: ");
		Serial.println(client.getLastErrorMessage());
	}
}

void my_homekit_loop() {
	arduino_homekit_loop();
	const uint32_t t = millis();
	if (t > next_report_millis) {
		// report sensor values every 10 seconds
		next_report_millis = t + 5 * 1000;
		my_homekit_report();
	}
	if (t > next_heap_millis) {
		// show heap info every 5 seconds
		next_heap_millis = t + 3 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}

void my_homekit_report() {
	float temperature_value = read_temperature();
	cha_current_temperature.value.float_value = temperature_value;
	LOG_D("Current temperature: %.1f", temperature_value);
	homekit_characteristic_notify(&cha_current_temperature, cha_current_temperature.value);

	float humidity_value = read_humidity();
	cha_current_humidity.value.float_value = humidity_value;
	LOG_D("Current humidity: %.1f%", humidity_value);
	homekit_characteristic_notify(&cha_current_humidity, cha_current_humidity.value);

	sgp.setHumidity(getAbsoluteHumidity(temperature_value, humidity_value));
	sgp.IAQmeasure();

	float tvoc = 4.5 * sgp.TVOC; // https://www.catsensors.com/media/pdf/Sensor_Sensirion_IAM.pdf section 2.3
	cha_tvoc.value.float_value = tvoc;
	LOG_D("Current tVOC: %.1f%", tvoc);
	homekit_characteristic_notify(&cha_tvoc, cha_tvoc.value);

	cha_co2.value.float_value = sgp.eCO2;
	LOG_D("Current CO2: %.1fppm", sgp.eCO2);
	homekit_characteristic_notify(&cha_co2, cha_co2.value);

	influx_report(temperature_value, humidity_value, sgp.eCO2, sgp.TVOC);
}

float read_humidity() {
	return sht31.readHumidity();
}

float read_temperature() {
	float t = sht31.readTemperature();
	// calibrated 2023-11-20
	float temp = t - 2;
	return t;
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
