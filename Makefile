.PHONY: all build upload serial help

CMD := arduino-cli
SERIAL_PORT := /dev/ttyUSB0
FQBN := esp8266:esp8266:huzzah

all: build upload

build: ## Compile the sketch.
	$(CMD) compile --fqbn $(FQBN)

upload: ## Upload the sketch to the esp8266
	$(CMD) upload -p $(SERIAL_PORT) --fqbn $(FQBN):baud=921600

serial: ## Connect to the device's serial monitor with screen.
	screen $(SERIAL_PORT) 115200

install-board: ## Install the esp8266 board
	$(CMD) core install --additional-urls http://arduino.esp8266.com/stable/package_esp8266com_index.json esp8266:esp8266 

install-libs: ## Install the required libraries
	# $(CMD) lib install "Adafruit MQTT Library"
	$(CMD) lib install "Adafruit SHT31 Library"
	$(CMD) lib install "Adafruit SGP30 Sensor"
	$(CMD) lib install "HomeKit-ESP8266"
	$(CMD) lib install "ESP8266 Influxdb"

help:
	@fgrep -h "##" $(MAKEFILE_LIST) | fgrep -v fgrep | sed -e 's/\\$$//' | sed -e 's/##//'
