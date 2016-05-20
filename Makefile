#####################################################################

# Set the arduino library directory.
ARDUINO_LIB_DIR = $(HOME)/Arduino/libraries

#####################################################################

COMPILER = g++
CFLAGS = -Wall -O -Iinclude
RFLAGS = `root-config --cflags --glibs`

# Directories
TOP_LEVEL = $(shell pwd)
INCLUDE_DIR = $(TOP_LEVEL)/include
SOURCE_DIR = $(TOP_LEVEL)/source
OBJ_DIR = $(TOP_LEVEL)/obj
EXEC_DIR = $(TOP_LEVEL)/exec
THERMOCOUPLE_LIB = $(TOP_LEVEL)/Adafruit_MAX31855.tar
THERMOCOUPLE_LIB_DIR = $(ARDUINO_LIB_DIR)/Adafruit_MAX31855
SDFAT_LIB = $(TOP_LEVEL)/greiman_SdFat.tar
SDFAT_LIB_DIR = $(ARDUINO_LIB_DIR)/greiman_SdFat

# PI serial library.
SERIAL_SRC = $(SOURCE_DIR)/wiringSerial.c
SERIAL_OBJ = $(OBJ_DIR)/wiringSerial.o

# Unpacker tool source.
UNPACKER_SRC = $(SOURCE_DIR)/loggerUnpacker.cpp
UNPACKER_EXE = $(EXEC_DIR)/loggerUnpacker

# File reader tool source.
READER_SRC = $(SOURCE_DIR)/csvReader.cpp
READER_EXE = $(EXEC_DIR)/csvReader

# Data logger processor tool source.
PROCESSOR_SRC = $(SOURCE_DIR)/processor.cpp
PROCESSOR_EXE = $(EXEC_DIR)/processor

########################################################################

all: install $(OBJ_DIR) $(EXEC_DIR) $(UNPACKER_EXE) $(READER_EXE) $(PROCESSOR_EXE)

########################################################################

$(OBJ_DIR):
#	Make the object file directory
	@if [ ! -d $@ ]; then \
		echo "Making directory: "$@; \
		mkdir $@; \
	fi

$(EXEC_DIR):
#	Make the object file directory
	@if [ ! -d $@ ]; then \
		echo "Making directory: "$@; \
		mkdir $@; \
	fi

########################################################################

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c
#	Compile C source files
	$(COMPILER) -c $(CFLAGS) -Iinclude $< -o $@

########################################################################

$(UNPACKER_EXE): $(SERIAL_OBJ) $(UNPACKER_SRC)
#	Compile unpacker tool.
	$(COMPILER) $(CFLAGS) -o $@ $(SERIAL_OBJ) $(UNPACKER_SRC)

$(READER_EXE): $(READER_SRC)
#	Compile unpacker tool.
	$(COMPILER) $(CFLAGS) -o $@ $(READER_SRC)

$(PROCESSOR_EXE): $(EXEC_DIR) $(PROCESSOR_SRC)
#	Compile unpacker tool.
	$(COMPILER) $(CFLAGS) -o $@ $(PROCESSOR_SRC) $(RFLAGS)

########################################################################

install:
#	Install the MAX31855 library from Adafruit.
#	And the SdFat library from https://github.com/greiman.
	@if [ -d $(ARDUINO_LIB_DIR) ]; then \
		cd $(ARDUINO_LIB_DIR); \
		if [ ! -d $(THERMOCOUPLE_LIB_DIR) ]; then \
			echo " Unpacking MAX31855 library files"; \
			tar -xf $(THERMOCOUPLE_LIB); \
		fi; \
		if [ ! -d $(SDFAT_LIB_DIR) ]; then \
			echo " Unpacking SdFat library files"; \
			tar -xf $(SDFAT_LIB); \
		fi; \
	else \
		echo " ERROR: directory "$(ARDUINO_LIB_DIR)" does not exist!"; \
	fi

uninstall:
	@echo " Removing installed library files"
	@rm -rf $(THERMOCOUPLE_LIB_DIR)/*
	@rmdir $(THERMOCOUPLE_LIB_DIR)
	@rm -rf $(SDFAT_LIB_DIR)/*
	@rmdir $(SDFAT_LIB_DIR)

########################################################################

clean:
	@echo " Removing compiled files"
	@rm -f $(OBJ_DIR)/*.o
	@rm -f $(EXEC_DIR)/*
