// #define DEBUG_MODE
// #define TMAX_OUT_PRINT
// #define IMAGE_BUILD
// #define GRAPH_BUILD
// // #define ACTIVATE_CLIQUES


// #define LAZY_CONS 20 
// #define PBSTR "------------------------------------------------------------"
// #define PBWIDTH 60


#include <iostream> 
#include <cstdlib>
#include <stdio.h>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <string>
#include <map>
#include <bits/stdc++.h>
#include <random>
#include <array>
#include <unistd.h>
#include <getopt.h>

using namespace std; 

#define no_argument 0
#define required_argument 1 
#define optional_argument 2

using std::cout; 
using std::string;

int nodeCount = 0 ;
int cliqueCount = 0 ;

string reset_edge = "1" ;

string top ="";
string instrumented = "inline.v";

string rst;
string clk;
string cycles;

int execute(std::string cmd, std::string& output) {
    const int bufsize=128;
    std::array<char, bufsize> buffer;
    auto pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    size_t count;
    do {
        if ((count = fread(buffer.data(), 1, bufsize, pipe)) > 0) {
            output.insert(output.end(), std::begin(buffer), std::next(std::begin(buffer), count));
        }
    } while(count > 0);
    return pclose(pipe);
} 

string files(vector<string>& allfiles){
    string filelist = R"()";

    for (int i = 0; i<allfiles.size() ; i++){
        filelist = filelist + R"(
read_verilog )" + allfiles[i] ;

    }

    return filelist;


}

string requesttb(string clk, string rst, string cycles, string resetedge){
    string maketb = R"(iverilog )" + instrumented +R"( -I ../includeFiles -B $midroot/Libraries/lib/ivl -t tbGenerate \
		-ptb=tb.v -pclk=)" + clk +R"( -preset=)" + rst +R"( -preset_edge=)" + resetedge +R"( \
		-punroll=)" + cycles +R"()";


    // iverilog aes.v -I ../includeFiles -B ~/Documents/Research/DARPA/del1/Malicious_Implant_Detection/libs/lib/ivl -t tbGenerate -ptb=tb.v -pclk=clk -preset=rst -preset_edge=1 -punroll=21
    return maketb;


}

void loginfo(string output)
{
  std::ofstream out(top +"_flatten.log", std::ios::app);
  out << output;
  out.close();
}



string flatten(string files,int effort){
string tclContent ;
if (effort==0)
{

tclContent = R"(
)" + files +R"(

# elaborate design hierarchy
hierarchy -check -top  )" + top +R"(

# the high-level stuff
proc; opt; fsm; opt; memory; opt

proc_mux
fsm; opt;

flatten 


proc_clean

select -module )" + top +R"(

# write inline design
write_verilog  -noattr -selected )" + instrumented +R"(

    )";
}

else
{

tclContent = R"(
)" + files +R"(

# elaborate design hierarchy
hierarchy -check -top  )" + top +R"(

# the high-level stuff
proc; opt; fsm; opt; memory; opt

#proc_mux
#fsm; opt;

flatten 
# splitnets -format __  )" + top +R"(

proc_rmdead
freduce 
opt_reduce -fine
opt_clean -purge

proc_clean

select -module )" + top +R"(

# write inline design
write_verilog  -noattr -selected )" + instrumented +R"(

    )";

}
    
    return tclContent;
}

void usage(string progName)
{
      cout << "=================================================================================  " << endl <<
    "FLATTEN : Remove the hierarchy from the hardware design and perform standardization" << endl <<
    "USAGE : " << progName << " [options]" << endl <<
    "Options:" << endl <<
    "-h | --help        Print this help" << endl <<
    "-v | --version     Print the version" << endl <<
    "-d | --design      Add each .v file with seperate -d flag" << endl <<
	"-f | --file      	Add each .v filename to a text file and pass with -f flag" << endl <<
    "-t | --top         Top module" << endl <<
    "-c | --cycles      Cycle depth" << endl <<
    "-k | --clk         Clock signal name" << endl <<
    "-s | --rst         Reset signal name" << endl <<
    "-e | --effort      Flatten optimization effort" << endl <<
	"-x | --resetedge   Reset edge for the design" << endl <<
    "================================================================================== " << endl ;

  exit(0);
}


int main(int argc, char **argv) 
{   

    vector<string> allfiles;

  const struct option longopts[] =
  {
    {"version",   no_argument,        0, 'v'},
    {"help",      no_argument,        0, 'h'},
    {"top",     required_argument,  0, 't'},
    {"design",     optional_argument,  0, 'd'},
	{"file",     optional_argument,  0, 'f'},
    {"instrumented",     optional_argument,  0, 'o'},
    {"cycles",     optional_argument,  0, 'c'},
    {"clk",     required_argument,  0, 'k'},
    {"rst",     required_argument,  0, 's'},
    {"effort",     optional_argument,  0, 'e'},
	{"resetedge",     optional_argument,  0, 'x'},
    {0,0,0,0},
  };

  int index;
  int iarg=0;
  int effort = 0;
  string file_name ;

  //turn off getopt error message
  opterr=1; 



  while(iarg != -1)
  {
    iarg = getopt_long(argc, argv, "t:d:o:s:c:k:r:e:f:x:vh", longopts, &index);

    switch (iarg)
    {
      case 't':
        top = optarg;
        break;

      case 'd':
        // design = optarg;
        allfiles.push_back(optarg);
        break;

	  case 'f':
		file_name = optarg;
        // design = optarg;
        // allfiles.push_back(optarg);
        break;

      case 'h':
        usage("./synth");
        break;

      case 'o':
        instrumented = optarg;
        break;
        

      case 'v':
        cout << "Automated synthesis tool for Trojan detection : Version 1.0";
        break;

      case 'c':
        cycles = std::to_string(stoi(optarg)-1);
        break;

      case 'k':
        clk = optarg;
        break;

      case 's':
        rst = optarg;
        break;
      
      case 'e':
        effort = stoi(optarg);
        break;

	  case 'x':
        reset_edge = optarg;
        break;
    }

	if (iarg == 'f')
	{

		string lines;
    	ifstream files(optarg);
    	while (getline (files, lines)) 
    	{
			allfiles.push_back(lines);
    	}
    	files.close();
	}

  } 

	
   
    
    string fileList =  files(allfiles);

    cout << "Flatten process started :";
    string flatten_script = flatten(fileList,effort);
    std::ofstream out("flatten.ys");
    out << flatten_script;
    out.close();
    std::string output;
    execute("yosys flatten.ys", output);
	loginfo(output);
	execute("sed  -i 's/\\./_/g' inline.v", output);
	loginfo(output);
	// cout << "sed -i -E 's/\[([0-9]*)\] ;+/_\\1_ ;/g' inline.v";
	// execute("sed -i -E 's/\\[([0-9]*)\\] +/_\\1_ /g' inline.v", output);
	// loginfo(output);
    execute(requesttb(clk,rst,cycles,reset_edge), output);
	loginfo(output);
    cout << "  Done \n";
   
	return 0; 
}