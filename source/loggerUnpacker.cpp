#include <iostream>
#include <fstream>
#include <string.h>
#include <cmath>

unsigned int timestamp;
float temperature;
float pressure;
short relay1;
short relay2;

int main(int argc, char *argv[]){
	if(argc < 2){ 
		std::cout << " Invalid number of arguments.\n";
		std::cout << "  SYNTAX: " << argv[0] << " <filename>\n";
		return 1; 
	}

	std::string ifname = std::string(argv[1]);
	std::ifstream file(ifname.c_str(), std::ios::binary);
	
	if(!file.is_open()){ return 1; }
	
	std::string ofname = ifname.substr(0, ifname.find_last_of('.'))+".csv";
	std::ofstream output(ofname.c_str());
	
	output << "time(ms)\tT(C)\tP(Torr)\tR1\tR2\n";
	
	unsigned int count = 0;
	while(true){
		file.read((char*)(&timestamp), 4);
		file.read((char*)(&temperature), 4);
		file.read((char*)(&pressure), 4);
		file.read((char*)(&relay1), 2);
		file.read((char*)(&relay2), 2);
		if(file.eof()){ break; }
		
		// Convert the pressure voltage to an actual pressure.
		pressure = pow(10.0, (pressure-5.0));
		
		output << timestamp << "," << temperature << "," << pressure << "," << relay1 << "," << relay2 << std::endl;
		
		count++;
	}
	
	std::cout << " Done! Read " << count << " data entries.\n";
	std::cout << "  Wrote output file '" << ofname << "\n";
	
	file.close();
	output.close();
	
	return 0;
}
