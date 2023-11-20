/*
 * my_accessory.c
 * Define the accessory in C language using the Macro in characteristics.h
 *
 *  Created on: 2020-05-15
 *      Author: Mixiaoxiao (Wang Bin)
 */

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

// Called to identify this accessory. See HAP section 6.7.6 Identify Routine
// Generally this is called when paired successfully or click the "Identify Accessory" button in Home APP.
void my_accessory_identify(homekit_value_t _value) {
	printf("accessory identify\n");
}

// For TEMPERATURE_SENSOR,
// the required characteristics are: CURRENT_TEMPERATURE
// the optional characteristics are: NAME, STATUS_ACTIVE, STATUS_FAULT, STATUS_TAMPERED, STATUS_LOW_BATTERY
// See HAP section 8.41 and characteristics.h

// (required) format: float; HAP section 9.35; min 0, max 100, step 0.1, unit celsius
homekit_characteristic_t cha_current_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);

// (optional) format: string; HAP section 9.62; max length 64
// homekit_characteristic_t cha_name = ;

// example for humidity
homekit_characteristic_t cha_current_humidity  = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);

homekit_characteristic_t cha_aq  = HOMEKIT_CHARACTERISTIC_(AIR_QUALITY, 0);

homekit_characteristic_t cha_tvoc  = HOMEKIT_CHARACTERISTIC_(VOC_DENSITY, 0);

homekit_characteristic_t cha_co2  = HOMEKIT_CHARACTERISTIC_(CARBON_DIOXIDE_LEVEL, 400);

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_sensor, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Huzzah SHT31-D SGP30"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Arduino HomeKit"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "1"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Temperature Sensor"),
            &cha_current_temperature,
            NULL
        }),
        // Add this HOMEKIT_SERVICE if you has a HUMIDITY_SENSOR together
        HOMEKIT_SERVICE(HUMIDITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Humidity Sensor"),
            &cha_current_humidity,
            NULL
        }),
        HOMEKIT_SERVICE(AIR_QUALITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Air quality sensor"),
            &cha_aq,
            &cha_tvoc,
            &cha_co2,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};
