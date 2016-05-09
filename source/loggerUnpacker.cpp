#include <iostream>
#include <fstream>
#include <string.h>

unsigned int timestamp;
float temperature;
float pressure;
short relay1;
short relay2;

int main(int argc, char *argv[]){
	if(argc < 2){ return 1; }

	std::ifstream file(argv[1], std::ios::binary);
	
	if(!file){ return 1; }
	
	std::cout << "time(ms)\tT(C)\tP(Torr)\tR1\tR2\n";
	
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
		
		std::cout << timestamp << "\t" << temperature << "\t" << pressure << "\t" << relay1 << "\t" << relay2 << std::endl;
		
		count++;
	}
	
	std::cout << "\n Done! Read " << count << " data entries.\n";
	
	file.close();
	
	return 0;
}
