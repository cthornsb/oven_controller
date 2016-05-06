#####################################################################

# Set the arduino library directory.
ARDUINO_LIB_DIR = $(HOME)/Arduino/libraries

#####################################################################

TOP_LEVEL = $(shell pwd)

THERMOCOUPLE_LIB = $(TOP_LEVEL)/Adafruit_MAX31855.tar

THERMOCOUPLE_LIB_DIR = $(ARDUINO_LIB_DIR)/Adafruit_MAX31855

all: install

install:
#	Install the MAX31855 library from Adafruit.
	@if [ ! -d $(THERMOCOUPLE_LIB_DIR) ]; then \
		echo " Unpacking thermocouple library files"; \
		cd $(ARDUINO_LIB_DIR); \
		tar -xf $(THERMOCOUPLE_LIB); \
	fi

clean:
	@echo " Removing thermocouple library files"
	@rm -rf $(THERMOCOUPLE_LIB_DIR)/*
	@rmdir $(THERMOCOUPLE_LIB_DIR)
