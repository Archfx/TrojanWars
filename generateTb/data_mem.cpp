#include <fstream>
#include <cassert>
#include "data_mem.h"
#include "globals.h"
#include <time.h>
#include <math.h>
using namespace std;

CTDataMem::CTDataMem(){
	unroll = 0;
	width = 0;
}


sig_pos* CTDataMem::add_input(string name, uint port_width){
	sig_pos* sig = new sig_pos;
	sig->lsb = width;
	sig->msb = width + port_width - 1;
	sig->width = port_width;
	in_ports[name] = sig;
	width += port_width;
	return sig;
}

//Dump data to file
void CTDataMem::dump(const char* file) {
    ofstream mem(file);
    for(auto line:data){
        mem << line << '\n';
    }
    mem.close();
}

void CTDataMem::generate() {
    
    unroll = g_unroll;
    assert(unroll);
    assert(width);
    data.clear();

    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    srand((int)floor(spec.tv_nsec/1.0e6)%1000000000000);
    // We want to use 16 bits from each rand()
    const uint count = width >> 4;
    const uint extra = width & 0b1111;
    for(uint k=0; k<=unroll; k++){
        string in_vector;
        
        uint num = rand();
        for(uint j=0; j < extra; j++){
            in_vector += '0' + (num & 1);
            num >>= 1;
        }
        for(uint i=0; i < count; i++){
            
            num = rand();
            for(uint j=0; j < 16; j++){
                in_vector += '0' + (num & 1);
                num >>= 1;
            }
        }
        data.push_back(in_vector);
    }
    dump(g_data_mem);
}