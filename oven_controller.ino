#include <SPI.h>
#include "SdFat.h"
#include "Adafruit_MAX31855.h"

//#define USE_SERIAL_ASCII
#define USE_SERIAL_BINARY

// Set the chip select pins.
#define THERMO_CHIPSELECT 4
#define SD_CHIPSELECT 10

// Set the communication pins for the thermocouple breakout.
// Only the CS pin needs to be set for the SD breakout.
#define DO_PIN 5
#define CLK_PIN 3

// Set the pressure gauge pin.
#define PRESSURE_PIN A0

// Set the relay control pins.
#define RELAY_1_IN 6
#define RELAY_2_IN 7
#define RELAY_1_OUT 8
#define RELAY_2_OUT 9

// Set the SD card detect pin.
#define CARD_DETECT_PIN 2

// Set the oven temperature control values (Celcius).
#define OVEN_MAX_TEMP 89.0
#define OVEN_MIN_TEMP 88.0
 
// Set the maximum oven pressure reading (V).
#define OVEN_MAX_PRESSURE 2.0017468
#define PUMPED_DOWN_PRESSURE 1.8511003

// Time (in milliseconds) to wait between read cycles.
#define READ_DELAY 1000

// Maximum time to write to SD file output (ms).
#define MAX_WRITE_TIME 86400000

// The delimiter between data packets.
const unsigned long delimiter = 0xFFFFFFFF;

// The time since the program started.
unsigned long timestamp = 0;
unsigned long new_time = 0;

// Variables to track 120V relay states.
int prev_relay1_state = 0;
int prev_relay2_state = 0;

int file_number = 1;
char filename[13] = "DATA0001.DAT";

bool sd_card_okay = false;
bool over_temp = false;
bool pumped_down = false;

// File system object.
SdFat sd;

// Log file.
SdFile file;

// initialize the Thermocouple.
Adafruit_MAX31855 thermocouple(CLK_PIN, THERMO_CHIPSELECT, DO_PIN);

// Write len_ bytes to the SD file.
void writeBytes(byte *val_, const byte &len_){
  if(!file.isOpen() || !val_ || len_==0){ return; }
  
  long container;
  memcpy((byte*)&container, val_, len_);
  
  byte current;
  for(byte index = 0; index < len_; index++){
    current = (container >> 8*index) & 0xFF;
    file.write(current);
  }
}

// Write len_ bytes to the Serial output.
void writeBytesSerial(byte *val_, const byte &len_){
#ifdef USE_SERIAL_BINARY
  if(!val_ || len_==0){ return; }
  
  long container;
  memcpy((byte*)&container, val_, len_);
  
  byte current;
  for(byte index = 0; index < len_; index++){
    current = (container >> 8*index) & 0xFF;
    Serial.write(current);
  }
#endif
}

void openFile(){
  if(!sd_card_okay){ return; }
  
  // Get the current filename.
  while(true){
    String temp_filename = "DATA";
    if(file_number < 10){ temp_filename += "000"; }
    else if(file_number < 100){ temp_filename += "00"; }
    else if(file_number < 1000){ temp_filename += "0"; }
    temp_filename += String(file_number) + ".DAT";

    temp_filename.toCharArray(filename, 14);

    file_number++;
    if (!sd.exists(filename)) { break; }
  }

#ifdef USE_SERIAL_ASCII    
  // Print the new filename to the serial monitor.
  Serial.print("Opening SD file ");
  Serial.print(filename);
  Serial.print("... ");
#endif
     
  // Open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (file.open(filename, O_CREAT | O_WRITE | O_EXCL)) {
    // Write the packet delimiter to start the file.
    writeBytes((byte*)&delimiter, 4);
#ifdef USE_SERIAL_ASCII     
    Serial.println("done.");
#endif
  }
  else{
#ifdef USE_SERIAL_ASCII     
    Serial.println("failed!");
#endif
  }
}

void initSD(){
  if(sd_card_okay){ return; }

#ifdef USE_SERIAL_ASCII     
  Serial.print("Initializing SD card... ");
#endif
  
  // Pin 10 must be left as output for SPI.
  pinMode(10, OUTPUT);

  if(sd.begin(SD_CHIPSELECT, SPI_HALF_SPEED)){
#ifdef USE_SERIAL_ASCII     
    Serial.println("done.");
#endif     
  }
  else{
#ifdef USE_SERIAL_ASCII     
    Serial.println("failed!");
#endif
    return;  
  }

  // Open a new data file.
  sd_card_okay = true;
  openFile();
}

void setup() {
  // Wait for MAX chip to stabilize
  delay(500);

  // Initialize the digital input pins.
  pinMode(RELAY_1_IN, INPUT);
  pinMode(RELAY_2_IN, INPUT);
  pinMode(CARD_DETECT_PIN, INPUT);
  
  // Initialize the digital output pins.
  pinMode(RELAY_1_OUT, OUTPUT);
  pinMode(RELAY_2_OUT, OUTPUT);

#if defined(USE_SERIAL_ASCII) || defined(USE_SERIAL_BINARY)
  // Open serial communications and wait for port to open.
  Serial.begin(9600);

  #ifdef USE_SERIAL_ASCII  
  // Inform the user that we've started.
  Serial.print("time(ms)\tT(C)\tP(V)\tR1\tR2\n");
  #endif
#endif

  // Check for inserted SD card.
  if(digitalRead(CARD_DETECT_PIN)){
    initSD();
  }
}

void loop() {
  // Get the current time (ms).
  timestamp = millis();

  // Read the digital pins.
  int relay1_state = digitalRead(RELAY_1_IN);
  int relay2_state = digitalRead(RELAY_2_IN);
  int card_detect = digitalRead(CARD_DETECT_PIN);

  // Read the temperature from the thermocouple.
  double temp = thermocouple.readCelsius();
  
  // Read the pressure from the pressure gauge.
  double pres = 5.0*analogRead(PRESSURE_PIN)/1023.0;
  
  // Do not allow the oven to run without the vacuum pump.
  if(relay2_state == 0){
    relay1_state = 0; // Disable the oven relay.
    pumped_down = false; // Reset the vacuum pressure interlock.
  }
  else if(pres <= PUMPED_DOWN_PRESSURE){
    pumped_down = true; // Start the vacuum pressure interlock.
  }
  
  // Check for oven over temp.
  if(!over_temp){
    if(temp >= OVEN_MAX_TEMP){
      // Force the oven off.
      relay1_state = 0;
      over_temp = true;
    }
    else{ relay1_state = digitalRead(RELAY_1_IN); }
  }
  else if(temp <= OVEN_MIN_TEMP){
    relay1_state = digitalRead(RELAY_1_IN);
    over_temp = false;
  }
  else{ relay1_state = 0; }
  
  // Check for over pressure state.
  if(pres > OVEN_MAX_PRESSURE){
    if(pumped_down){
      // LOSS OF VACUUM PRESSURE! EMERGENCY PUMP SHUTDOWN!
      relay2_state = 0;
    }
    // Shut down the oven if the pressure is too high.
    relay1_state = 0;
  }
  
  // Set the relay states.
  if(relay1_state != prev_relay1_state){
    digitalWrite(RELAY_1_OUT, relay1_state); 
    prev_relay1_state = relay1_state;
  }
  if(relay2_state != prev_relay2_state){
    digitalWrite(RELAY_2_OUT, relay2_state);
    prev_relay2_state = relay2_state;
  }    
     
  // Hot-swappable SD card handler.
  if(timestamp <= MAX_WRITE_TIME && (!sd_card_okay && card_detect == 1)){ // Check for newly inserted SD card.
    initSD();
  }
  else if(timestamp > MAX_WRITE_TIME || (sd_card_okay && card_detect == 0)){ // Check for removed SD card.
    sd_card_okay = false;
    
    // Force data to SD and update the directory entry to avoid data loss.
    if (!file.sync() || file.getWriteError()) {
#ifdef USE_SERIAL_ASCII
      Serial.println("Failed to sync to SD file!");
#endif
    }
    
    // Close the SD file.
    file.close();
  }

  // Write to file/serial.
  if(sd_card_okay){ // Write to the sd card.
    // Write the packet delimiter.
    writeBytes((byte*)&delimiter, 4);

    // Write the time.
    writeBytes((byte*)&timestamp, 4);
  
    // Write the temperature.
    writeBytes((byte*)&temp, 4);
  
    // Write the pressure.
    writeBytes((byte*)&pres, 4);
  
    // Write the relay states.
    writeBytes((byte*)&relay1_state, 2);
    writeBytes((byte*)&relay2_state, 2);
  }
#ifdef USE_SERIAL_ASCII
  // Print the time.
  Serial.print(timestamp);
  Serial.print("\t");
  
  // Print the temperature.
  if(isnan(temp)){ Serial.print("nan"); }
  else{ Serial.print(temp); }
  Serial.print("\t");
  
  // Print the pressure.
  Serial.print(pres);
  Serial.print("\t");
  
  // Print the relay states.
  Serial.print(relay1_state);
  Serial.print("\t");
  Serial.print(relay2_state);
  Serial.print("\n");
#elif defined(USE_SERIAL_BINARY)
  // Write the packet delimiter.
  writeBytesSerial((byte*)&delimiter, 4);

  // Write the time.
  writeBytesSerial((byte*)&timestamp, 4);

  // Write the temperature.
  writeBytesSerial((byte*)&temp, 4);

  // Write the pressure.
  writeBytesSerial((byte*)&pres, 4);

  // Write the relay states.
  writeBytesSerial((byte*)&relay1_state, 2);
  writeBytesSerial((byte*)&relay2_state, 2);
#endif
  
  // Check if a delay is needed.
  new_time = millis();
  if(new_time-timestamp < READ_DELAY){
    delay(READ_DELAY-(new_time-timestamp));
  }
}
