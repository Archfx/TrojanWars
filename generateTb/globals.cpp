#include "ivl_target.h"
#include "data_mem.h"
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>

using namespace std;

//global variables
int				g_errors = 0;
unsigned		g_ind = 0;
unsigned		g_ind_incr = 4;
ivl_design_t	g_design = 0;
const char*     g_tb_file = "tb.v";
const char*     g_input_file = "inputs.txt";
const char*     g_data_mem = "data.mem";
const char*     g_sig_file = "nodes.txt";
CTDataMem		g_data;
clock_t         start_time;

//Parameters
uint			g_unroll;
const char*     g_clock_sig_name;
const char*     g_reset_sig_name;
const char*     g_reset_edge_active;
