#include <iostream>
#include <fstream>
#include <deque>
#include <string.h>
#include <cmath>
#include <stdio.h>
#include <unistd.h>
#include <sstream>
#include <stdlib.h>

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

size_t serialGets(const int &fd_, char *buf_, const size_t &len_){
	int numBytes = read(fd_, buf_, len_-1);
	if(numBytes >= 0){ buf_[len_-1] = '\0'; }
	return numBytes;
}

size_t serialRead(const int &fd_, char *val_, const size_t &len_){
	return read(fd_, val_, len_);
}

size_t serialRead(const int &fd_, std::deque<char> &data_, const size_t &len_){
	char dummy;
	size_t values = 0;
	for(size_t i = 0; i < len_; i++){
		if(read(fd_, &dummy, 1) == 1){ 
			data_.push_back(dummy); 
			values++;
		}
	}
	return values;
}

void getDataWord(const std::deque<char> &data_, const size_t &offset_, char *val_, const size_t &len_=4){
	if(offset_+len_ > data_.size()){ return; }
	
	char temp[len_];
	for(size_t i = 0; i < len_; i++){
		temp[i] = data_[offset_+i];
	}
	memcpy(val_, temp, len_);
}

void popQueue(std::deque<char> &data_, const size_t &len_){
	size_t count = 0;
	while(!data_.empty() && count < len_){
		data_.pop_front();
		count++;
	}
}

void replaceChar(char *str_, const size_t &len_, const char &c1_, const char &c2_){
	for(size_t index = 0; index < len_; index++){
		if(str_[index] == c1_){ str_[index] = c2_; }
	}
}

// Return the order of magnitude of a number
float getOrder(const float &input_, int &power){
	float test = 1E-10;
	for(int i = -10; i < 10; i++){
		if(input_/test <= 1){ 
			power = i;
			return test; 
		}
		test *= 10.0;
	}
	return 1.0;
}

// Return a scientific notation representation of an input number.
std::string sciNotation(const float &input_, const size_t &N_=2){
	int power = 0;
	double order = getOrder(input_, power);
	
	std::stringstream stream;
	stream << 10*input_/order;
	
	// Limit to N_ decimal places due to space constraints
	std::string output = stream.str();
	size_t find_index = output.find('.');
	if(find_index != std::string::npos){
		std::string temp;
		temp = output.substr(0, find_index);
		temp += output.substr(find_index, N_+1);
		output = temp;
	}

	std::stringstream stream2;
	stream2 << output << "E" << power-1; 
	output = stream2.str();

	return output;
}

void help(char * prog_name_){
	std::cout << "  SYNTAX: " << prog_name_ << " <filename> [options] [output]\n";
	std::cout << "   Available options:\n";
	std::cout << "    --print      | Print unpacked data to stdout.\n";
	std::cout << "    --serial     | Read data from a serial port.\n";
	std::cout << "    --ascii      | Read ascii from the serial port.\n";
	std::cout << "    --ping <num> | Ping serial port and display readings.\n";
}

int main(int argc, char *argv[]){
	if(argc < 2){ 
		std::cout << " Invalid number of arguments to " << argv[0] << ". Expected 1, received " << argc-1 << ".\n";
		help(argv[0]);
		return 1;
	}
	
	std::string ifname = std::string(argv[1]);
	std::string ofname = ifname.substr(0, ifname.find_last_of('.'))+".csv";
	bool serial_mode = false;
	bool ascii_mode = false;
	bool ping_mode = false;
	bool printout = false;
	int num_ping = -1;
	int index = 2;
	while(index < argc){
		if(strcmp(argv[index], "--print") == 0){
			printout = true;
		}
		else if(strcmp(argv[index], "--serial") == 0){
			std::cout << " Using serial mode.\n";
			serial_mode = true;
		}
		else if(strcmp(argv[index], "--ascii") == 0){
			std::cout << " Using ascii mode.\n";
			ascii_mode = true;
			serial_mode = true;
		}
		else if(strcmp(argv[index], "--ping") == 0){
			if(index + 1 >= argc){
				std::cout << " Error! Missing required argument to '--ping'!\n";
				help(argv[0]);
				return 1;
			}
			num_ping = atoi(argv[++index]);
			serial_mode = true;
			ping_mode = true;
			printout = true;
			if(num_ping <= 0){
				std::cout << " Error! Number of ping attempts must be greater than zero!\n";
				return 1;
			}
			std::cout << "--Pinging device " << num_ping << " times.--\n";
		}
		else{ // Unrecognized command, must be the output filename.
			ofname = std::string(argv[index]); 
		}
		index++;
	}

	// Load the output file.
	std::ofstream output;
	
	if(!ping_mode){
		output.open(ofname.c_str(), std::ios::binary);
		if(!output.is_open()){ 
			std::cout << " ERROR: Failed to open output file '" << ofname << "'!\n";
			return 1; 
		}
	}

	// Load the input file.
	std::ifstream file;
	int fd = 0;

	if(!serial_mode){
		file.open(argv[1], std::ios::binary);
		if(!file.is_open()){ 
			std::cout << " ERROR: Failed to open input file '" << argv[1] << "'!\n";
			output.close();
			return 1; 
		}
	}
	else{
		fd = serialOpen(argv[1], 9600);
		if(fd < 0){
			std::cout << " ERROR: Failed to open serial port '" << argv[1] << "'!\n";
			output.close();
			return 1; 
		}
		else{ std::cout << " Connected to " << argv[1] << " (fd=" << fd << ")\n"; }
	}

	setup_signal_handlers();
	
	output << "time(ms),T(C),P(Torr),R1,R2\n";
	
	bool firstRun = true;
	unsigned int count = 0;
	std::deque<char> data;
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
			if(bytesReady == 0){ // Not enough bytes waiting to be read.
				// Wait for 10 ms for some bytes to read.
				usleep(10000);
				continue;
			}
			else if(bytesReady < 0){ // Error on port.
				std::cout << " ERROR: Encountered error on serial port!\n";
				break;
			}		

			if(ping_mode && num_ping-- <= 0){
				break;
			}

			if(!ascii_mode){ // Reading binary from serial.
				// Check that we are not out of sync by up to 3 bytes.
				// Check for 4 0xFF bytes in a row. This will signify
				// the beginning of a data packet.
				if(serialRead(fd, data, bytesReady) < 0){
					std::cout << " ERROR: Encountered error reading on serial port!\n";
					break;
				}

				if(data.size() >= 20){ // Scan the data.				
					while(!data.empty()){
						if(SIGNAL_INTERRUPT){
							break;
						}
				
						unsigned int dummy;
						getDataWord(data, 0, (char*)&dummy);
						if(dummy == 0xFFFFFFFF){
							break;
						} 
						else{ data.pop_front(); }
					}
					
					if(data.size() >= 20){ // Read the data.
						getDataWord(data, 0, (char*)&delimiter);
						getDataWord(data, 4, (char*)&timestamp);
						getDataWord(data, 8, (char*)&temperature);
						getDataWord(data, 12, (char*)&pressure);
						getDataWord(data, 16, (char*)&relay1, 2);
						getDataWord(data, 18, (char*)&relay2, 2);
						
						// Remove the data from the queue.
						popQueue(data, 20);
					}
					else{ continue; }
				}
				else{ continue; }
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

				// Delete the string.
				delete[] msg;

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

		// Get a string of the pressure in scientific notation.
		std::string pressureString = sciNotation(pressure);

		// Print data to the screen.
		if(printout){
			if(ping_mode){ std::cout << " ping " << num_ping+1 << ":"; }
			std::cout << " time = " << timestamp/1000 << " s, temp = " << temperature << " C, pres = ";
			std::cout << pressureString << " Torr, R1 = " << relay1 << ", R2 = " << relay2;
			if(!ping_mode){ std::cout << "\r" << std::flush; }
			else{ std::cout << "\n"; }
		}
		
		if(!ping_mode){
			// Write ascii data to the output file.
			output << timestamp << ",";
			output << temperature << ",";
			output << pressureString << ",";
			output << relay1 << ",";
			output << relay2 << "\n";
		}

		count++;
	}
	
	std::cout << "\n Done! Read " << count << " data entries.\n";
	
	// Close the input file/port.
	if(!serial_mode){ file.close(); }
	else{ serialClose(fd); }

	if(!ping_mode){
		std::cout << "  Wrote output file '" << ofname << "'\n";
		output.close();
	}

	return 0;
}
