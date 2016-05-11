#include <iostream>
#include <fstream>
#include <string.h>
#include <cmath>

#include <signal.h>
#include <stdexcept>

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

void help(char * prog_name_){
	std::cout << "  SYNTAX: " << prog_name_ << " <filename> [options] [output]\n";
	std::cout << "   Available options:\n";
	std::cout << "    --print | Print unpacked data to stdout.\n";
}

int main(int argc, char *argv[]){
	if(argc < 2){ 
		std::cout << " Invalid number of arguments to " << argv[0] << ". Expected 1, received " << argc-1 << ".\n";
		help(argv[0]);
		return 1;
	}

	bool printout = false;
	int index = 2;
	while(index < argc){
		if(strcmp(argv[index], "--print") == 0){
			printout = true;
		}
		index++;
	}

	// Load the input file.
	std::string ifname = std::string(argv[1]);
	std::ifstream file(ifname.c_str(), std::ios::binary);

	if(!file.is_open()){ 
		std::cout << " ERROR: Failed to open input file '" << ifname << "'!\n";
		return 1; 
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
		file.close();
		return 1; 
	}

	setup_signal_handlers();
	
	output << "time(ms),T(C),P(Torr),R1,R2";
	
	unsigned int count = 0;
	while(true){
		if(SIGNAL_INTERRUPT){
			break;
		}
	
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
		
		// Print data to the screen.
		if(printout){
			std::cout << " time = " << timestamp/1000 << " s, temp = " << temperature << " C, pres = " << pressure/1000 << " mTorr, R1 = " << relay1 << ", R2 = " << relay2 << "\r";
			std::cout << std::flush;
		}
		
		// A voltage divider is used in order to get the full
		// range of 1-8 V from the pressure gauge using the
		// 5 V arduino. Convert the pressure voltage to the real voltage.
		pressure = pressure/RPRIME;
		
		// Convert the pressure voltage to an actual pressure.
		pressure = pow(10.0, (pressure-5.0));
		
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
	
	file.close();
	output.close();
	
	return 0;
}
