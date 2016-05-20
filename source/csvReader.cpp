#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <sstream>

void help(char * prog_name_){
	std::cout << "  SYNTAX: " << prog_name_ << " <rawFile>\n";
}

int main(int argc, char* argv[]){
	if(argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)){
		help(argv[0]);
		return 1;
	}
	else if(argc < 2){
		std::cout << " Error: Invalid number of arguments to " << argv[0] << ". Expected 1, received " << argc-1 << ".\n";
		help(argv[0]);
		return 1;
	}
	
	std::ifstream input(argv[1]);
	if(!input.good()){
		return 1;
	}
	
	std::ofstream output("tmp.dat");

	int msTime;
	std::string line;	
	unsigned int count = 0;
	while(true){
		getline(input, line);
		if(input.eof()){ break; }
	
		if(count == 0){ // Replace the file header with a new one.
			output << "milliseconds,temperature,pressure,r1,r2,seconds\n";
			count++;
			continue;
		}
		else if(count == 1){ // Skip the first line of data as it is usually junk.
			count++;
			continue;
		}
		
		count++;
		if(count % 10000 == 0 && count != 0){ std::cout << "  Line " << count << " of data file\n"; }
		
		if(line.find("nan") != std::string::npos){
			std::cout << " Found nan on line " << count << std::endl;
			continue;
		}
		else if(line.find("inf") != std::string::npos){
			std::cout << " Found inf on line " << count << std::endl;
			continue;
		}
		
		msTime = atoi(line.substr(0, line.find(',')).c_str());
		
		std::stringstream stream;
		stream << msTime/1000;
		
		output << line << "," << stream.str() << std::endl;
	}
	
	input.close();
	output.close();
	
	return 0;
}
