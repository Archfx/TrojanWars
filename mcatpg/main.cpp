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

using namespace std; 

using std::cout; 
using std::string;

int nodeCount = 0 ;
int cliqueCount = 0 ;

string DESIGN = "c3540"; // Benchmark design


//  =============== Console functions =============== 

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

void printProgress(double percentage) {
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

//  =============== ATPG functions =============== 


string tclSaveModel(){
    string tclContent = R"(
set_messages -nodisplay
set_commands noabort
cd /home/UFAD/arunajayasena/Designs/MaximalClique/Benchmarks
set readFile ")" + DESIGN +R"(/)" + DESIGN +R"(.v"
set topModule ")" + DESIGN +R"("
set writeFile ")" + DESIGN +R"(/)" + DESIGN +R"(_output.v"
set testPatterns ")" + DESIGN +R"(/)" + DESIGN +R"(_patterns.stil"
set libFile "Libraries/saed90nm.v"
read_netlist $readFile
read_netlist $libFile -library
#report_modules -summary
#report_modules -error
#report_modules -undefined
run_build_model $topModule
run_drc
set_faults -model stuck
write_image )" + DESIGN +R"(/)" + DESIGN +R"(.image -replace
exit
    )";

    return tclContent;
}


string tclQuery(string constraint, string stuck){

    string tclContent = R"(
set_messages -nodisplay
set_commands noabort
cd /home/UFAD/arunajayasena/Designs/MaximalClique/Benchmarks
read_image )" + DESIGN +R"(/)" + DESIGN +R"(.image
#add_faults -all
)" + constraint + R"(
)" + stuck + R"(
#list down the faults 
# for scan chain designs
set_atpg -merge high -full_seq_time {60 240} -full_seq_atpg
run_atpg
set_messages -display
report_patterns -summary -internal
#write_patterns $testPatterns -format stil -replace
exit
    )";

    return tclContent;
}

string tclCliques(vector<int>& Clique, vector<pair<string, string>>& rareNodeMap){

    string constraints = "";
    string stucks = "";
    for (int j = 0 ; j<Clique.size(); j++) 
        {
            string val = get<1>(rareNodeMap[Clique[j]]) == "1" ? "0" : "1" ;
            // cout << Clique[j] << ", " ;
            constraints = constraints + "\nadd_atpg_constraints MC"+ std::to_string(Clique[j]) + " " + get<1>(rareNodeMap[Clique[j]]) + R"( -module )" + DESIGN +R"( )" + get<0>(rareNodeMap[Clique[j]]) ;
            stucks = stucks + "\nadd_faults  -stuck  "+ val + " " + get<0>(rareNodeMap[Clique[j]]) ;
            

        }
    // cout << constraints ;
    // cout << stucks ;

    string tclContent = R"(
set_messages -nodisplay
set_commands noabort
cd /home/UFAD/arunajayasena/Designs/MaximalClique/Benchmarks
read_image )" + DESIGN +R"(/)" + DESIGN +R"(.image
#add_faults -all
)" + constraints + R"(
)" + stucks + R"(
#list down the faults 
# for scan chain designs
set_atpg -merge high -verbose -full_seq_time {600 3600} -full_seq_atpg
run_atpg
set_messages -display
report_patterns -all -internal
#write_patterns $testPatterns -format stil -replace
exit
    )";

    return tclContent;
}

string getConstraint(pair<string, string> node1, pair<string, string> node2)
{
    string atpg_constraint = R"(
add_atpg_constraints a )"+ get<1>(node1) + R"( -module )" + DESIGN +R"( )" + get<0>(node1) + R"(
add_atpg_constraints b )"+ get<1>(node2) + R"( -module )" + DESIGN +R"( )" + get<0>(node2) + R"(
    )";
    return atpg_constraint;
}

string getStuck(pair<string, string> node1, pair<string, string> node2)
{
    string val1 = get<1>(node1) == "1" ? "0" : "1" ;
    string val2 = get<1>(node2) == "1" ? "0" : "1" ;
    string stuck_fault = R"(
add_faults  -stuck  )"+ val1 + R"( )" + get<0>(node1) + R"(
add_faults  -stuck  )"+ val2 + R"( )" + get<0>(node2) + R"(
    )";
    return stuck_fault;
} 

//  =============== Support functions ===============

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

vector<int> split2int (string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<int> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (std::stoi(token));
    }

    res.push_back (std::stoi(s.substr (pos_start)));
    return res;
}

string removeSpaces(string input)
{
  input.erase(std::remove(input.begin(),input.end(),' '),input.end());
  input.erase(std::remove(input.begin(),input.end(),'\n'),input.end());
  return input;
}

//  =============== I/O functions ===============

void getRareNodes(vector<pair<string, string>>& rareNodeMap)
{

    string nodes;
    ifstream rareValues("Benchmarks/" + DESIGN +"/rare_nodes.txt");

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
}

void getCliques(vector<vector <int>>& allCliques)
{

    string clique;
    ifstream cliques("Benchmarks/" + DESIGN +"/cliques.g");

    while (getline (cliques, clique)) 
    {
        vector<int> v = split2int (clique, " ");
        allCliques.push_back(v);
        cliqueCount ++;
    }

    cliques.close();
}



int main() 
{ 
    vector<pair<string, string>> rareNodeMap;
    getRareNodes(rareNodeMap);

    #ifdef DEBUG_MODE 
    cout << rareNodeMap.size();
    cout << "INDEX\tELEMENT\tVALUE\n";
    for (int i = 0 ; i<rareNodeMap.size(); i++) {
        cout << i
             << '\t' << get<0>(rareNodeMap[i]) << '\t' << get<1>(rareNodeMap[i]) << '\n';
    }
    cout << nodeCount;
    #endif

    #ifdef IMAGE_BUILD
        cout << "\nBuilding the model";
        // Write the drc model for future use
        string write_image = tclSaveModel();
        std::ofstream out("buildModel.tcl");
        out << write_image;
        out.close();
        std::string output;
        execute("tmax -shell -tcl buildModel.tcl", output);
        cout << " : Done \n";
    #endif

    #ifdef GRAPH_BUILD 

    #ifdef TMAX_OUT_PRINT
        cout << output;
    #endif


    string graphEdges = "";
    int edgeCount = 0;
    int i, j = 0;
    int query_count = 0;
    cout << "Generating connectivity Graph : \n";

    for (int i = 0; i < nodeCount; i++)
    {
        pair<string, string> node1 = rareNodeMap[i];
        for (int j = 0; j < i; j++) 
        {
           
            pair<string, string> node2 = rareNodeMap[j];
            
            #ifdef DEBUG_MODE
            cout << " Node pair " << i <<","<< j << "\n";
            cout << getConstraint(node1,node2);
            #endif


            string tclContent = tclQuery(getConstraint(node1,node2),getStuck(node1,node2));
        
            std::ofstream out("query.tcl");
            out << tclContent;
            out.close();

            // Read console outputs
            std::string output;

            execute("tmax -shell -tcl query.tcl", output);

            #ifdef TMAX_OUT_PRINT
            std::cout << output;
            #endif

            if (output.find("#internal patterns                           0") != std::string::npos) 
            {
                // cout << "No edge between " << get<0>(node1) << " and "<< get<0>(node2) << '\n';
            }
            else 
            {
                // cout << "Edge between " << get<0>(node1) << " and "<< get<0>(node2) << '\n';
                graphEdges = graphEdges + to_string(i) + "," + to_string(j) + "\n";
                edgeCount++;
                // cout << edgeCount;
            }

            query_count ++;
            printProgress((double) query_count/(double)(nodeCount*(nodeCount-1)*0.5));
            

            
                
        }
    }
    cout << "\n";
    std::ofstream graph("Benchmarks/" + DESIGN +"/graph.g");
    graph << to_string(nodeCount) + "\n" + to_string(edgeCount) + "\n" + graphEdges;
    graph.close();
    #endif 

    #ifdef ACTIVATE_CLIQUES 
    cout << "Reading Cliques File : ";

    vector<vector <int>> allCliques;
    getCliques(allCliques);

    #ifdef DEBUG_MODE 
    cout << allCliques.size();
    cout << "CLIQUE\n";
    for (int i = 0 ; i<allCliques.size(); i++) 
    {
        for (int j = 0 ; j<allCliques[i].size(); j++) 
        {
            cout << allCliques[i][j] << ", " ;
        }
        cout << "\n" ;
    }
    cout << nodeCount;
    #endif

    cout << "Done \n";

    cout << "Generating Test Vectors for Activating Cliques : \n";  

    ofstream patternFile("Benchmarks/" + DESIGN +"/" + DESIGN +".patterns");

    for (int i = 0 ; i<allCliques.size(); i++) 
    {
        string genVectors = tclCliques(allCliques[i], rareNodeMap);
        std::ofstream actCliq("activateCliques.tcl");
        actCliq << genVectors;
        actCliq.close();
        std::string outputCliq;
        execute("tmax -shell -tcl activateCliques.tcl", outputCliq);

        #ifdef TMAX_OUT_PRINT
        cout << outputCliq;
        #endif

        if (outputCliq.find("Time 0: force_all_pis =") != std::string::npos) 
        {
            #ifdef DEBUG_MODE
            cout << removeSpaces(split(split(outputCliq, "force_all_pis =")[1]," Time 1: measure_all_pos =")[0]) << '\n';
            #endif

            if(patternFile.is_open())
            {
                patternFile<<removeSpaces(split(split(outputCliq, "force_all_pis =")[1]," Time 1: measure_all_pos =")[0])<< endl;  
            }
            else cerr<<"Unable to open file";
        }
        printProgress((double) i/(double)(allCliques.size()));
    }

    cout << "\nWriting test vectors to " + DESIGN +".patterns file: ";
    cout << " : Done \n";
    patternFile.close();

    #endif
    


	return 0; 
} 