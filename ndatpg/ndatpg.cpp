// #define DEBUG_MODE
// #define TMAX_OUT_PRINT
// #define IMAGE_BUILD
// #define GRAPH_BUILD
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
#include <sstream>

#define no_argument 0
#define required_argument 1 
#define optional_argument 2

using namespace std; 

using std::cout; 
using std::string;

string top;
string design = "synthesised.v";
string rarenodes = "rare_nodes.txt";
int ndetect = 10;

int nodeCount = 0 ;
int cliqueCount = 0 ;
int unroll_cycle = 3;

vector<string> pis;
string rst;
string clk;

// declares
string removeSpaces(string input);


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
  cout << "================================================================================" << endl <<
      "NDATPG : Activate rare nodes with statistical Ndetect with Synopsys TetraMax" << endl <<
      "USAGE : " << progName << " [options]" << endl <<
      "Options:" << endl <<
      "-h | --help        Print this help" << endl <<
      "-v | --version     Print the version" << endl <<
      "-d | --Design      Synthesized Design for tmax" << endl <<
      "-t | --top         Top module" << endl <<
      "-c | --cycles      Cycle depth" << endl <<
      "-n | --ndetect     Statistical NDetect Criterion" << endl <<
      "-r | --rarenodes   Rare nodes file" << endl <<
      "-k | --clk         Clock signal name" << endl <<
      "-s | --rst         Reset signal name" << endl <<
      "================================================================================" << endl ;

  exit(0);
}



string tclSaveModel(){
    string tclContent = R"(
set_messages -nodisplay
set_commands noabort
set readFile ")" + design +R"("
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


string tclQuery(string constraint, string stuck, string piconstraint){

    string tclContent = R"(
set_messages -nodisplay
set_commands noabort
read_image design.image
#add_faults -all
drc
)" + piconstraint + R"(
test
set_faults -model stuck
)" + constraint + R"(
)" + stuck + R"(
#list down the faults 
# for scan chain designs
set_atpg -merge high -full_seq_time {60 240} -full_seq_atpg
run_atpg
set_messages -display
report_patterns -all -internal
#write_patterns $testPatterns -format stil -replace
exit
    )";

    return tclContent;
}






string getConstraint(pair<string, string> node1)
{
    string atpg_constraint = R"(
add_atpg_constraints a )"+ get<1>(node1) + R"( -module )" + top +R"( )" + get<0>(node1) + R"(
    )";
    return atpg_constraint;
}

string getStuck(pair<string, string> node1)
{
    string val1 = removeSpaces(get<1>(node1)) == "1" ? "0" : "1" ;
    string stuck_fault = R"(
add_faults  -stuck  )"+ val1 + R"( )" + get<0>(node1) + R"(
    )";
    return stuck_fault;
} 

string getPiconstraint(string val, int index)
{
    string pi_constraint = R"(
add_pi_constraints )"+ val + R"( )"+ pis[index] + R"(
    )";
    return pi_constraint;
} 


//  =============== Support functions ===============

void loginfo(string output)
{
  std::ofstream out(top +"_ndatpg.log", std::ios::app);
  out << output;
  out.close();
  if (output.find("Error:") != std::string::npos && output.find("(M5)") != std::string::npos && output.find("(M377)") != std::string::npos && output.find("(M19)") != std::string::npos)  
  {
    cout << "\nERROR : Please refer the following TetraMax error. Please refer the usage details of NTATPG below\n";
    cout << " **** TetraMax error : " << output << " ***\n";
    usage("./ndatpg");
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

void replace_txt( std::string& s, std::string const& toReplace, std::string const& replaceWith) 
{

    std::size_t pos = s.find(toReplace);
    s.replace(pos, toReplace.length(), replaceWith);

}

int min(int x, int y)
{
  return (x < y) ? x : y;
}

string slicePattern(string word, int index1, int index2)
{
  string word2 = (word.erase(index1,1)).erase(index2-1,1);
  return word2;
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
			//cout << nodes;
			
            vector<string> v = split (nodes, ": ");


            if (v[0].find("[") != std::string::npos & v[0].find("]") != std::string::npos) 
            {

              string nodename = replace_txt(v[0], "[","_");
              v[0] = replace_txt(nodename, "]","_");
          
            }
			
			if (v.size()>1)
			{	
	            rareNodeMap.push_back(make_pair(v[0], removeSpaces(v[1])));
	            nodeCount ++;
			}
        }
    }

    rareValues.close();
}

void printProgress(double percentage) {
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

string removeSpaces(string input)
{
  input.erase(std::remove(input.begin(),input.end(),' '),input.end());
  input.erase(std::remove(input.begin(),input.end(),'\n'),input.end());
  return input;
}

vector<string> getPis(){

    string tclContent = R"(
set_messages -nodisplay
set_commands noabort
read_image design.image
set_messages -display
report_primitives -pis
exit
    )";

    std::ofstream out("query.tcl");
    out << tclContent;
    out.close();

    // Read console outputs
    std::string output;
 

    execute("tmax -shell -tcl query.tcl", output);
    loginfo(output);
    std::istringstream iss(output);
    for (std::string line; std::getline(iss, line); )
    {
      if (line.find("PI") != std::string::npos) 
          {
            pis.push_back (split(split(line,"PI  ")[1]," ")[0]);
          }
    }

    return pis;
}

bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(), 
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}





int main(int argc, char **argv) 
{   

  const struct option longopts[] =
  {
    {"version",   no_argument,        0, 'v'},
    {"help",      no_argument,        0, 'h'},
    {"top",     required_argument,  0, 't'},
    {"design",     optional_argument,  0, 'd'},
    {"rarenodes",     optional_argument,  0, 'r'},
    {"ndetect",     optional_argument,  0, 'n'},
    {"cycles",     optional_argument,  0, 'c'},
    {"clk",     required_argument,  0, 'k'},
    {"rst",     required_argument,  0, 's'},
    {0,0,0,0},
  };

  int index;
  int iarg=0;



  opterr=1; 



  while(iarg != -1)
  {
    iarg = getopt_long(argc, argv, "t:d:r:c:n:k:s:vh", longopts, &index);

    switch (iarg)
    {
      case 't':
        top = optarg;
        break;

      case 'd':
        design = optarg;
        break;

      case 'h':
        usage("./ndatpg");
        break;
      
      case 'r':
        rarenodes = optarg;
        break;

      case 'n':
        ndetect = stoi(optarg);
        break;

      case 'c':
        unroll_cycle = stoi(optarg);
        break;

      case 'k':
        clk = optarg;
        break;

      case 's':
        rst = optarg;
        break;

      case 'v':
        cout << "NDATPG : Activate rare nodes with statistical Ndetect with Synopsys TetraMax tool" << endl << "Version 1.0";
        break;
    }

  }

    cout << "================================================================================" << endl <<
     "\nNDATPG : Activate rare nodes with statistical Ndetect with Synopsys TetraMax\n";


    // cout << "\nBuilding the model";
    // // Write the drc model for future use
    // string write_image = tclSaveModel();
    // std::ofstream out("buildModel.tcl");
    // out << write_image;
    // out.close();
    // std::string output;
    // execute("tmax -shell -tcl buildModel.tcl", output);
    // loginfo(output);
    // cout << " : Done \n";
    

    cout << "PI extraction";
    getPis();

    int clk_index = 0;
    int rst_index = 0;

    // cout << pis.size();

    for (int i = 0; i < pis.size(); i++)
    {

      if (pis[i]==clk) clk_index = i;
      if (pis[i]==rst) rst_index = i;
    }

    for (int i = 0; i < pis.size(); i++)
    {

      if (pis[i]==clk) pis.erase(pis.begin()+i);
    }

    for (int i = 0; i < pis.size(); i++)
    {

      if (pis[i]==rst) pis.erase(pis.begin()+i);
    }

    // for (int i = 0; i < pis.size(); i++)
    // {

    //   cout << pis[i];
    // }
    // cout<< clk_index << "clk" << clk << pis[0] << rst_index << "rst" << rst << pis[1] ;

    // cout << pis.size();

    cout << " : Done \n";

    cout << "Generating test vectors \n";

    vector<pair <string, string>> rareNodeMap;
    getRareNodes(rareNodeMap, rarenodes);


    string rarelist = "";
    string assign = "";
    int interCount = 0;

    ofstream patternFile( top +"_ndatpg.patterns", std::ios_base::app);
	
 
    for (int i = 0; i < nodeCount; i++)
    {

        string rarenode = get<0>(rareNodeMap[i]);
        string rarevalue = get<1>(rareNodeMap[i]);


        if (rarenode.find("[") != std::string::npos & rarenode.find("]") != std::string::npos) 
        {
          
          replace_txt(rarenode, "[","_");
          replace_txt(rarenode, "]","_");
          
        }

        pair<string, string> node = (make_pair(rarenode, rarevalue));

        string tclContent = tclQuery(getConstraint(node),getStuck(node),"");
          
        std::ofstream out("query.tcl");
        out << tclContent;
        out.close();

        // Read console outputs
		std::string output;

        execute("tmax -shell -tcl query.tcl", output);
        loginfo(output);
        #ifdef TMAX_OUT_PRINT
        std::cout << output;
        #endif

		if (output.find("Pattern 0 (full_sequential)") != std::string::npos) 
          {
              
              vector<string> patterns = split (output, ": force_all_pis =");
              vector<string> vectors ;
              vector<int> indices;
              int period ;
              for (int  i =0 ; i< patterns.size();i++)
              {
                if (patterns[i].find("period =") != std::string::npos) period = stoi(removeSpaces(split(patterns[i],"period =")[1]));
                if (is_number(split (patterns[i], " ").back()))
                {
                // cout << " clock "<< stoi(split (patterns[i], " ").back())/period << endl;
                indices.push_back(stoi(split (patterns[i], " ").back())/period);
                }
                if (patterns[i].find("measure_all_pos =") != std::string::npos) 
                {
                  // cout << slicePattern(removeSpaces(split(patterns[i]," Time ")[0]),0,1) << endl;
                  vectors.push_back(slicePattern(removeSpaces(split(patterns[i]," Time ")[0]),0,1));
                }
              }

              //for (int v; v<indices.size();v++)
              //{
                //cout << indices[v] << endl;
              //}

              int index = 0;
              int ptr = 0;

              for (int y=0; y<unroll_cycle; y++)
              {
                  if (indices[ptr]==y) 
                  {
                    // index = y;
                    ptr++;
                  }
                  //cout << y <<"  : " <<vectors[ptr-1] << " : "<<  ptr <<endl;
				  if(patternFile.is_open())
              	  {
                  patternFile<<vectors[ptr-1]<< endl;  
                  }
                  else cerr<<"Unable to open file";
                  }

          }  


        else if (output.find("Time 0: force_all_pis =") != std::string::npos) 
          {
              string pattern = slicePattern(removeSpaces(split(split(output, "force_all_pis =")[1]," Time 1: measure_all_pos =")[0]),clk_index,rst_index).substr(0,pis.size());
              #ifdef DEBUG_MODE
                cout << pattern << '\n';
                cout << pattern.size();
              #endif
              

              if(patternFile.is_open())
              {
                  patternFile<<pattern<< endl;  
              }
              else cerr<<"Unable to open file";

              for (int n = 0;  n < min(pis.size(), ndetect); n++)
              {
                if (pattern[n]=='0') tclContent = tclQuery(getConstraint(node),getStuck(node),getPiconstraint("1",  n));
                else tclContent = tclQuery(getConstraint(node),getStuck(node),getPiconstraint("0",  n));
                std::ofstream out("query_n.tcl");
                out << tclContent;
                out.close();
                std::string output_n;
                execute("tmax -shell -tcl query_n.tcl", output_n);
                loginfo(output_n);

				if (output_n.find("Pattern 0 (full_sequential)") != std::string::npos) 
          		{
              
              		vector<string> patterns = split (output_n, ": force_all_pis =");
              		vector<string> vectors ;
              		vector<int> indices;
              		int period ;
              		for (int  i =0 ; i< patterns.size();i++)
               		{
              			if (patterns[i].find("period =") != std::string::npos) period = stoi(removeSpaces(split(patterns[i],"period =")[1]));
              			if (is_number(split (patterns[i], " ").back()))
              			{
              				// cout << " clock "<< stoi(split (patterns[i], " ").back())/period << endl;
              				indices.push_back(stoi(split (patterns[i], " ").back())/period);
              			}
              			if (patterns[i].find("measure_all_pos =") != std::string::npos) 
              			{
              				// cout << slicePattern(removeSpaces(split(patterns[i]," Time ")[0]),0,1) << endl;
              				vectors.push_back(slicePattern(removeSpaces(split(patterns[i]," Time ")[0]),0,1));
              			}
              }


              int index = 0;
              int ptr = 0;

              for (int y=0; y<unroll_cycle; y++)
              {
                  if (indices[ptr]==y) 
                  {
                    // index = y;
                    ptr++;
                  }
                  //cout << y <<"  : " <<vectors[ptr-1] << " : "<<  ptr <<endl;
				  if(patternFile.is_open())
              	  {
                  patternFile<<vectors[ptr-1]<< endl;  
                  }
                  else cerr<<"Unable to open file";
                  }

          		}  
                else if (output_n.find("Time 0: force_all_pis =") != std::string::npos) 
                {
                    string pattern_n = slicePattern(removeSpaces(split(split(output_n, "force_all_pis =")[1]," Time 1: measure_all_pos =")[0]),clk_index,rst_index).substr(0,pis.size());
                    #ifdef DEBUG_MODE
                      cout << pattern_n << '\n';
                    #endif

                    if(patternFile.is_open())
                    {
                      patternFile<<pattern_n<< endl;
                      for (int m = 0;  m < unroll_cycle-1;)
                      {
                        if (pattern_n[m%pis.size()]=='0') tclContent = tclQuery(getConstraint(node),getStuck(node),getPiconstraint("1",  m%pis.size()));
                        else tclContent = tclQuery(getConstraint(node),getStuck(node),getPiconstraint("0",  m%pis.size()));
                        std::ofstream out("query_n.tcl");
                        out << tclContent;
                        out.close();
                        std::string output_m;
                        execute("tmax -shell -tcl query_n.tcl", output_m);
                        loginfo(output_m);

                        if (output_m.find("Time 0: force_all_pis =") != std::string::npos) 
                        {
                          string pattern_m = slicePattern(removeSpaces(split(split(output_m, "force_all_pis =")[1]," Time 1: measure_all_pos =")[0]),clk_index,rst_index).substr(0,pis.size());
                          patternFile<<pattern_m<< endl; 
                          m++;
                        }
						else
						{
						  patternFile<<pattern_n<< endl;
						  m++;
						}

                      } 
                    }
                    else cerr<<"Unable to open file";
                }
                interCount++;
                printProgress((double) (interCount)/(double)(nodeCount*min(pis.size(), ndetect)));

              }
          }
        else  interCount = interCount + min(pis.size(), ndetect);
        
		printProgress((double) (interCount)/(double)(nodeCount*min(pis.size(), ndetect)));

        // query_count ++;
        // printProgress((double) query_count/(double)(nodeCount*(nodeCount-1)*0.5));
        
    }
	printProgress(1);
    patternFile.close();
    cout << " : Done \n";

    cout <<"\nNDATPG Generated Test Vectors saved to " + top +"_ndatpg.patterns " + "\n";

    cout << "================================================================================\n";
   
	return 0; 
}