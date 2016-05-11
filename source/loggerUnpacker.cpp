#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <cmath>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <signal.h>
#include <stdexcept>

#include "wiringSerial.h"

#define RPRIME 0.5004367

unsigned int delimiter;
unsigned int timestamp;
float temperature;
float pressure;
short relay1;
short relay2;

bool SIGNAL_INTERRUPT = false;

void sig_int_handler(int ignore_){
	SIGNAL_INTERRUPT = true;
}

// Setup the interrupt signal intercept
void setup_signal_handlers(){ 
	// Handle ctrl-c press (SIGINT)
	if(signal(SIGINT, SIG_IGN) != SIG_IGN){	
		if(signal(SIGINT, sig_int_handler) == SIG_ERR){
			throw std::runtime_error(" Error setting up SIGINT signal handler!");
		}
	}
}

int serialOpenB(const char *device, const int baud, const int max=1000){
	struct termios options ;
	int fd = serialOpen(device, baud);
	
	// Get and modify current options:
	tcgetattr(fd, &options);
	
	options.c_lflag |= ICANON; // set canonical mode (line by line)
	options.c_iflag |= IGNCR; // ignore CR on input
	options.c_cc [VMIN] = max-1; // return if max-1 bytes received
	options.c_cc [VTIME] = 0; // no timeout
	
	tcsetattr(fd, TCSANOW | TCSAFLUSH, &options);
	
	usleep(10000); // 10mS
	
	return fd ;
}

size_t serialGets(const int &fd_, char *buf_, const size_t &len_){
	int numBytes = read(fd_, buf_, len_-1);
	if(numBytes >= 0){ buf_[len_-1] = '\0'; }
	return numBytes;
}

size_t serialRead(const int &fd_, char *val_, const size_t &len_){
	return read(fd_, val_, len_);
}

void replaceChar(char *str_, const size_t &len_, const char &c1_, const char &c2_){
	for(size_t index = 0; index < len_; index++){
		if(str_[index] == c1_){ str_[index] = c2_; }
	}
}

void help(char * prog_name_){
	std::cout << "  SYNTAX: " << prog_name_ << " <filename> [options] [output]\n";
	std::cout << "   Available options:\n";
	std::cout << "    --print  | Print unpacked data to stdout.\n";
	std::cout << "    --serial | Read data from a serial port.\n";
	std::cout << "    --ascii  | Read ascii from the serial port.\n";
}

int main(int argc, char *argv[]){
	if(argc < 2){ 
		std::cout << " Invalid number of arguments to " << argv[0] << ". Expected 1, received " << argc-1 << ".\n";
		help(argv[0]);
		return 1;
	}

	bool serial_mode = false;
	bool ascii_mode = false;
	bool printout = false;
	int index = 2;
	while(index < argc){
		if(strcmp(argv[index], "--print") == 0){
			printout = true;
		}
		if(strcmp(argv[index], "--serial") == 0){
			serial_mode = true;
		}
		if(strcmp(argv[index], "--ascii") == 0){
			ascii_mode = true;
			serial_mode = true;
		}
		index++;
	}

	// Load the input file.
	std::string ifname = std::string(argv[1]);
	std::ifstream file;
	int fd;

	if(!serial_mode){
		file.open(argv[1], std::ios::binary);
		if(!file.is_open()){ 
			std::cout << " ERROR: Failed to open input file '" << argv[1] << "'!\n";
			return 1; 
		}
	}
	else{
		fd = serialOpenB(argv[1], 9600);
		if(fd < 0){
			std::cout << " ERROR: Failed to open serial port '" << argv[1] << "'!\n";
			return 1; 
		}
		else{ std::cout << " Connected to " << argv[1] << " (fd=" << fd << ")\n"; }
	}

	std::string ofname;
	if(argc > 2){ // User specified output filename.
		ofname = std::string(argv[2]);
	}
	else{ // Get the output filename from the input filename.
		ofname = ifname.substr(0, ifname.find_last_of('.'))+".csv";
	}
	
	std::ofstream output(ofname.c_str());

	if(!output.is_open()){ 
		std::cout << " ERROR: Failed to open output file '" << ofname << "'!\n";
		if(!serial_mode){ file.close(); }
		else{ serialClose(fd); }
		return 1; 
	}

	setup_signal_handlers();
	
	output << "time(ms),T(C),P(Torr),R1,R2\n";
	
	bool firstRun = true;
	unsigned int count = 0;
	while(true){
		if(SIGNAL_INTERRUPT){
			break;
		}

		if(!serial_mode){ // Reading from a binary file. Standard operation.
			file.read((char*)(&delimiter), 4);
		
			if(file.eof()){ break; }
	
			// Check for the beginning of a new packet.
			if(delimiter != 0xFFFFFFFF){ continue; }
	
			// Read data from the file.
			file.read((char*)(&timestamp), 4);
			file.read((char*)(&temperature), 4);
			file.read((char*)(&pressure), 4);
			file.read((char*)(&relay1), 2);
			file.read((char*)(&relay2), 2);
		
			if(file.eof()){ break; }
		}
		else{ // Reading from serial.
			if(firstRun){
				// Flush whatever is waiting on the port.
				serialFlush(fd);
				firstRun = false;
			}

			int bytesReady = serialDataAvail(fd);
			if(bytesReady == 0){ // No bytes waiting to be read.
				// Wait for 10 ms for some bytes to read.
				usleep(10000);
				continue;
			}
			else if(bytesReady < 0){ // Error on port.
				std::cout << " ERROR: Encountered error on serial port!\n";
				break;
			}			

			if(!ascii_mode){ // Reading binary from serial.
				if(serialRead(fd, (char*)&delimiter, 4) < 0){
					std::cout << " ERROR: Encountered error reading on serial port!\n";
					break;
				}
		
				// Check for the beginning of a new packet.
				if(delimiter != 0xFFFFFFFF){ continue; }
		
				// Read data from the file.
				if(serialRead(fd, (char*)&timestamp, 4) < 0 ||
				   serialRead(fd, (char*)&temperature, 4) < 0 ||
				   serialRead(fd, (char*)&pressure, 4) < 0 ||
				   serialRead(fd, (char*)&relay1, 2) < 0 ||
				   serialRead(fd, (char*)&relay2, 2) < 0){
					std::cout << " ERROR: Encountered error reading on serial port!\n";
					break;
				}
			}
			else{ // Reading ascii from serial.
				char *msg = new char[bytesReady+1];
				if(serialGets(fd, msg, bytesReady+1) < 0){
					std::cout << " ERROR: Encountered error reading on serial port!\n";
					break;
				}
	
				// Print data to the screen.
				if(printout){
					printf("%s", msg);
				}

				// Do no processing of the incoming data.
				// Insert commas and write to the output file.
				replaceChar(msg, bytesReady+1, '\t', ',');
				output << msg;

				// Do no further processing.
				count++;
				continue;
			}
		}
		
		// A voltage divider is used in order to get the full
		// range of 1-8 V from the pressure gauge using the
		// 5 V arduino. Convert the pressure voltage to the real voltage.
		pressure = pressure/RPRIME;
	
		// Convert the pressure voltage to an actual pressure.
		pressure = pow(10.0, (pressure-5.0));

		// Print data to the screen.
		if(printout){
			std::cout << " time = " << timestamp/1000 << " s, temp = " << temperature << " C, pres = " << pressure*1000 << " mTorr, R1 = " << relay1 << ", R2 = " << relay2 << "\r";
			std::cout << std::flush;
		}
		
		// Write ascii data to the output file.
		output << timestamp << ",";
		output << temperature << ",";
		output << pressure << ",";
		output << relay1 << ",";
		output << relay2 << "\n";

		count++;
	}
	
	std::cout << "\n Done! Read " << count << " data entries.\n";
	std::cout << "  Wrote output file '" << ofname << "'\n";
	
	// Close the input file/port.
	if(!serial_mode){ file.close(); }
	else{ serialClose(fd); }

	output.close();

	return 0;
}