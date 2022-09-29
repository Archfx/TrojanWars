// #define DEBUG_MODE
// #define TMAX_OUT_PRINT
#define IMAGE_BUILD
#define GRAPH_BUILD
// #define ACTIVATE_CLIQUES


#define LAZY_CONS 20 
#define PBSTR "------------------------------------------------------------"
#define PBWIDTH 60


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

#define no_argument 0
#define required_argument 1 
#define optional_argument 2

using namespace std; 

using std::cout; 
using std::string;

int nodeCount = 0 ;
int cliqueCount = 0 ;
string top;

string preSynth = "preSynth.v";


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

void usage(string progName)
{
  cout << progName << "[options]" << endl <<
      "Options:" << endl <<
      "-h | --help        Print this help" << endl <<
      "-v | --version     Print the version" << endl <<
      "-d | --Design     Flattened Design for synthesis" << endl <<
      "-t | --top   Top module" << endl <<
      "-r | --rarenodes   rare_nodes.txt" << endl ;
  exit(0);
}

string requesttb(){
    string maketb = R"(dc_shell -f synth.tcl)";


    // iverilog aes.v -I ../includeFiles -B ~/Documents/Research/DARPA/del1/Malicious_Implant_Detection/libs/lib/ivl -t tbGenerate -ptb=tb.v -pclk=clk -preset=rst -preset_edge=1 -punroll=21
    return maketb;


}


string synth(string top, string design){
    string tclContent = R"(

set_messages -nodisplay

set_commands noabort

set readFile  )" + preSynth +R"(

set topModule )" + top +R"(

set writeFile "synthesised.v"

set_app_var search_path "."

set_app_var target_library [getenv techmap]/saed90nm_typ.db

set_app_var link_library [getenv techmap]/saed90nm_typ.db


read_file -format verilog $readFile

current_design $topModule

link

create_clock clk -period 10

compile -map_effort medium

set_flatten

write -hierarchy -format verilog -output $writeFile

exit

    )";

    return tclContent;
}

//  =============== Support functions ===============

void loginfo(string output)
{
  std::ofstream out(top +"_synth.log", std::ios::app);
  out << output;
  out.close();
  if (output.find("Error:") != std::string::npos && output.find("(M5)") != std::string::npos && output.find("(M377)") != std::string::npos && output.find("(M19)") != std::string::npos)  
  {
    cout << "\nERROR : Please refer the following TetraMax error. Please refer the usage details of NTATPG below\n";
    cout << " **** TetraMax error : " << output << " ***\n";
    usage("./synth");
    exit(0);
  }



}

vector<string> split (string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}


//  =============== I/O functions ===============

void getRareNodes(vector<pair<string, string>>& rareNodeMap, string rare_nodes)
{

    string nodes;
    ifstream rareValues(rare_nodes);

    while (getline (rareValues, nodes)) 
    {
        if (nodes.find(":") != std::string::npos) 
        {
            vector<string> v = split (nodes, ": ");
            rareNodeMap.push_back(make_pair(v[0], v[1]));
            nodeCount ++;
        }
    }

    rareValues.close();
	if (nodeCount == 0)
	{
		cout << "No rare nodes found, Please re-run the rareanalyser with different rareness threshold"<< endl;
		exit(0);
	}
}

string replace_txt( std::string& s, std::string const& toReplace, std::string const& replaceWith) 
{
    string outs = s;
    std::size_t pos = outs.find(toReplace);
    if (pos == std::string::npos) return outs;
    outs.replace(pos, toReplace.length(), replaceWith);

    return outs;
}

string tclSaveModel(){
    string tclContent = R"(
set_messages -nodisplay
set_commands noabort
set readFile  synthesised.v 
set topModule ")" + top +R"("
#set writeFile "design_output.v"
set testPatterns "test_patterns.stil"
set libFile [getenv techmap]/saed90nm.v
read_netlist $readFile
read_netlist $libFile -library
#report_modules -summary
#report_modules -error
#report_modules -undefined
run_build_model $topModule
run_drc
set_faults -model stuck
write_image design.image -replace
exit
    )";

    return tclContent;
}



int main(int argc, char **argv) 
{   

  
  string design = "inline.v";
  string rarenodes = "rare_nodes.txt";
  


  const struct option longopts[] =
  {
    {"version",   no_argument,        0, 'v'},
    {"help",      no_argument,        0, 'h'},
    {"top",     required_argument,  0, 't'},
    {"design",     optional_argument,  0, 'd'},
    {"rarenodes",     optional_argument,  0, 'r'},
    {0,0,0,0},
  };

  int index;
  int iarg=0;

  //turn off getopt error message
  opterr=1; 



  while(iarg != -1)
  {
    iarg = getopt_long(argc, argv, "t:d:r:vh", longopts, &index);

    switch (iarg)
    {
      case 't':
        top = optarg;
        break;

      case 'd':
        design = optarg;
        break;

      case 'h':
        usage("./synth");
        break;
      
      case 'r':
        rarenodes = optarg;
        break;

      case 'v':
        cout << "Automated synthesis tool for Trojan detection : Version 1.0";
        break;
    }

  }




    vector<pair<string, string>> rareNodeMap;
    getRareNodes(rareNodeMap, rarenodes);

	cout << "Preparing pre-synthesis instrumentation " ;

    string rarelist = "";
    string assign = "";

	std::string output;

	execute("cp "+ design + " temp.v", output);

    for (int i = 0; i < nodeCount; i++)
    {
      string node = get<0>(rareNodeMap[i]);
      if (node.find("[") != std::string::npos & node.find("]") != std::string::npos) 
      {
		string rhs =  " = " + node + ";\n";
        string nodename = replace_txt(node, "[","_");
        node = replace_txt(nodename, "]","_");
        rarelist = rarelist + node +",";
        assign = assign + "assign " + node + rhs;
      }
      else 
	  {
			rarelist = rarelist + node +",\n";
			execute("sed -i 's/wire "+node+";//g' temp.v", output);
      		execute("sed -i 's/output "+node+";//g' temp.v", output);
      		execute("sed -i 's/, "+node+"//g' temp.v", output);
      }
    }



    rarelist.pop_back();
    // rarelist.pop_back();


     //cout << assign;

    string lines;
    string outDesign = "";
    bool findinterface = true;


	
    ifstream design2syn("temp.v");
    while (getline (design2syn, lines)) 
    {

        if (lines.find(");") != std::string::npos & findinterface) 
        {
          findinterface = false;
          lines = replace_txt(lines, ");", ","+ rarelist +");");
          outDesign = outDesign + lines + "\n\n\n";
          outDesign = outDesign + "output " + rarelist + "; \n";
          outDesign = outDesign + assign + "\n";   
		  //preSynth << outDesign << endl;        
        }
        else 
        {
          outDesign = outDesign + lines + "\n";
	      //preSynth << outDesign<< endl;
        } 

         //cout << lines ;
    }

    design2syn.close();


    std::ofstream preSynth("preSynth.v");
    preSynth << outDesign;
    preSynth.close();
	execute("rm temp.v", output);
	cout << " : Done" << endl;
	//cout << "Removing duplicate entries from instrumentation " ;

    //std::string output;

 /*   for (int i = 0; i < nodeCount; i++)
    {
      string node = get<0>(rareNodeMap[i]);
	  if (lines.find("[") == std::string::npos) 
        {
			cout << "removing redefinition of" << node ;
      		execute("sed -i 's/wire "+node+";//g' preSynth.v", output);
      		execute("sed -i 's/output "+node+";//g' preSynth.v", output);
      		execute("sed -i 's/, "+node+"//g' preSynth.v", output);
		}
    }

	cout << " : Done" << endl;
*/

    cout << "Synthesis process started :";
    string synth_script = synth(top,design);
    std::ofstream out("synth.tcl");
    out << synth_script;
    out.close();
    
    execute("dc_shell -f synth.tcl", output);
	  loginfo(output);
    cout << "  Done \n"; 

    cout << "\nBuilding the ATPG model";
    // Write the drc model for future use
    string write_image = tclSaveModel();
    std::ofstream buildmodel("buildModel.tcl");
    buildmodel << write_image;
    buildmodel.close();
    execute("tmax -shell -tcl buildModel.tcl", output);
    loginfo(output);
    cout << " : Done \n";
    


   
	return 0; 
}