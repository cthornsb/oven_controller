#include <SPI.h>
#include "SdFat.h"
#include "Adafruit_MAX31855.h"

#define USE_SERIAL

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

// Set the oven temperature control values.
#define OVEN_MAX_TEMP 81.0
#define OVEN_MIN_TEMP 78.0

// Time (in seconds) to wait between writes.
#define WRITE_WAIT 2

// The time since the program started.
unsigned long time;

// Variables to track 120V relay states.
int prev_relay1_state = 0;
int prev_relay2_state = 0;

int file_number = 1;
char filename[13] = "DATA0001.DAT";

bool sd_card_okay = false;
bool over_temp = false;

// File system object.
SdFat sd;

// Log file.
SdFile file;

// initialize the Thermocouple.
Adafruit_MAX31855 thermocouple(CLK_PIN, THERMO_CHIPSELECT, DO_PIN);

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

#ifdef USE_SERIAL     
  // Print the new filename to the serial monitor.
  Serial.print("Opening SD file ");
  Serial.print(filename);
  Serial.print("... ");
#endif
     
  // Open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (file.open(filename, O_CREAT | O_WRITE | O_EXCL)) {
#ifdef USE_SERIAL     
    Serial.println("done.");
#endif
    file.print("time(s)\tT(C)\tP(mTorr)\tR1\tR2\n");
  }
  else{
#ifdef USE_SERIAL     
    Serial.println("failed!");
#endif
  }
}

void initSD(){
  if(sd_card_okay){ return; }

#ifdef USE_SERIAL     
  Serial.print("Initializing SD card... ");
#endif
  
  // Pin 10 must be left as output for SPI.
  pinMode(10, OUTPUT);

  if(sd.begin(SD_CHIPSELECT, SPI_HALF_SPEED)){
#ifdef USE_SERIAL     
    Serial.println("done.");
#endif     
  }
  else{
#ifdef USE_SERIAL     
    Serial.println("failed!");
#endif
    return;  
  }

  // Open a new data file.
  sd_card_okay = true;
  openFile();
}

void setup() {
  // Initialize the digital input pins.
  pinMode(RELAY_1_IN, INPUT);
  pinMode(RELAY_2_IN, INPUT);
  pinMode(CARD_DETECT_PIN, INPUT);
  
  // Initialize the digital output pins.
  pinMode(RELAY_1_OUT, OUTPUT);
  pinMode(RELAY_2_OUT, OUTPUT);

#ifdef USE_SERIAL
  // Open serial communications and wait for port to open.
  Serial.begin(9600);
  
  // Inform the user that we've started.
  Serial.print("time(s)\tT(C)\tP(mTorr)\tR1\tR2\n");
#endif

  // Check for inserted SD card.
  if(digitalRead(CARD_DETECT_PIN)){
    initSD();
  }
    
  // Wait for MAX chip to stabilize
  delay(500);
}

void loop() {
  int relay1_state = digitalRead(RELAY_1_IN);
  int relay2_state = digitalRead(RELAY_2_IN);
  int card_detect = digitalRead(CARD_DETECT_PIN);

  // Read the temperature from the thermocouple.
  double temp = thermocouple.readCelsius();
  
  // Read the pressure from the pressure gauge.
  double pres = (analogRead(PRESSURE_PIN)/1023.0)*5.0;
  
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
  
  // Set the relay states.
  if(relay1_state != prev_relay1_state){
    digitalWrite(RELAY_1_OUT, relay1_state); 
    prev_relay1_state = relay1_state;
  }
  if(relay2_state != prev_relay2_state){
    digitalWrite(RELAY_2_OUT, relay2_state);
    prev_relay2_state = relay2_state;
  }    
  
  // Get the current time (s).
  time = millis()/1000;
    
  // Hot-swappable SD card handler.
  if(!sd_card_okay && card_detect == 1){ // Check for newly inserted SD card.
    initSD();
  }
  else if(sd_card_okay && card_detect == 0){ // Check for removed SD card.
    sd_card_okay = false;
    file.close();
  }
  
  // Check if it's time to write to file/serial.
  if(time % WRITE_WAIT == 0){    
    if(sd_card_okay){ // Write to the sd card.
      // Print the time.
      file.print(time);
      file.print("\t");
    
      // Print the temperature.
      if(isnan(temp)){ file.print("nan"); }
      else{ file.print(temp); }
      file.print("\t");
    
      // Print the pressure.
      file.print(pres);
      file.print("\t");
    
      // Print the relay states.
      file.print(relay1_state);
      file.print("\t");
      file.print(relay2_state);
      file.print("\n");
      
      // Force data to SD and update the directory entry to avoid data loss.
      if (!file.sync() || file.getWriteError()) {
#ifdef USE_SERIAL
        Serial.println("Failed to flush to SD file!");
#endif
      }
    }
#ifdef USE_SERIAL
    // Print the time.
    Serial.print(time);
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
#endif
  }
  
  delay(1000);
}
