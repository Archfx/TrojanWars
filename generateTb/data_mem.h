#pragma once
#include <string>
#include <vector>
#include <map>
#include "types.h"

typedef struct{
    uint msb;
    uint lsb;
    uint width;
}sig_pos;

//Class for managing data file
class CTDataMem{
public:	
    CTDataMem();
    
    //Add input signal name to in_ports
    sig_pos* add_input(std::string name, uint port_width);
	
	inline uint get_width() {return width;}
	
    //Input ports mapped with their name
	std::map<std::string, sig_pos*> in_ports;	

    //Dump data structure to a file
    void dump(const char* dest_file);
    
    //Generate random bits in data, then dump to file
    void generate();
    
private:
    std::vector<std::string> data;			 //structure for holding data    
    uint width;
    uint unroll;
};
