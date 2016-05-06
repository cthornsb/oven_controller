#####################################################################

# Set the arduino library directory.
ARDUINO_LIB_DIR = $(HOME)/Arduino/libraries

#####################################################################

COMPILER = g++
CFLAGS = -Wall -O -Iinclude

# Directories
TOP_LEVEL = $(shell pwd)
SRC_DIR = $(TOP_LEVEL)/source
THERMOCOUPLE_LIB = $(TOP_LEVEL)/Adafruit_MAX31855.tar
THERMOCOUPLE_LIB_DIR = $(ARDUINO_LIB_DIR)/Adafruit_MAX31855
SDFAT_LIB = $(TOP_LEVEL)/greiman_SdFat.tar
SDFAT_LIB_DIR = $(ARDUINO_LIB_DIR)/greiman_SdFat

# Unpacker tool source.
UNPACKER_SRC = $(SRC_DIR)/loggerUnpacker.cpp
UNPACKER_EXE = $(TOP_LEVEL)/loggerUnpacker

all: install

unpacker: $(UNPACKER_SRC)
#	Compile unpacker tool.
	$(COMPILER) $(CFLAGS) -o $(UNPACKER_EXE) $(UNPACKER_SRC)

install:
#	Install the MAX31855 library from Adafruit.
	@if [ ! -d $(THERMOCOUPLE_LIB_DIR) ]; then \
		echo " Unpacking MAX31855 library files"; \
		cd $(ARDUINO_LIB_DIR); \
		tar -xf $(THERMOCOUPLE_LIB); \
	fi
#	Install the SdFat library from https://github.com/greiman.
	@if [ ! -d $(SDFAT_LIB_DIR) ]; then \
		echo " Unpacking SdFat library files"; \
		cd $(ARDUINO_LIB_DIR); \
		tar -xf $(SDFAT_LIB); \
	fi

clean:
	@echo " Removing installed library files"
	@rm -rf $(THERMOCOUPLE_LIB_DIR)/*
	@rmdir $(THERMOCOUPLE_LIB_DIR)
	@rm -rf $(SDFAT_LIB_DIR)/*
	@rmdir $(SDFAT_LIB_DIR)
	@echo " Removing compiled files"
	@rm -f $(UNPACKER_EXE)
