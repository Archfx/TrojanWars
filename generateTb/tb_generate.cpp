#include "globals.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <valarray>
#include <sys/stat.h>
#include <iostream>


using namespace std;




static void generate_tb(ivl_scope_t root);
static void pre_processing(ivl_design_t design);
static void extract_parameters(ivl_design_t design);
static void generate_sig_file(ivl_scope_t root);
void random_sim();
void instrument_design();

extern "C" int target_design(ivl_design_t design) {
	start_time = clock();
    //set globals
	g_design = design;
    
    pre_processing(design);
	
	//Get root module
	ivl_scope_t *roots;
	unsigned nroots;
	ivl_design_roots(design, &roots, &nroots);
	if(nroots != 1){
		error("Number of root is: %u. Use flattenverilog to flatten the hierarchy first", nroots);
	}
	if(ivl_scope_type(roots[0]) != IVL_SCT_MODULE){
		error("Root scope type is not module");
	}
	
    generate_tb(roots[0]);

    generate_sig_file(roots[0]);

    random_sim();

    instrument_design();
	
	return g_errors;
}

extern "C" const char* target_query(const char*key) {
	if (strcmp(key, "version") == 0) {
		return "Conquest 1.0";
	}

	return 0;
}

//Things that needs to be done before instrumentation and testbench generation starts
void pre_processing(ivl_design_t design){
    //extract parameters passed to iverilog target
    extract_parameters(design);
    
}


void extract_parameters(ivl_design_t design){
    //extract unroll cycles
    const char* unroll_cyc = ivl_design_flag(design, "unroll");
	if(unroll_cyc == NULL){
		error("Unroll cycle not given");
	}
	g_unroll = atoi(unroll_cyc);
    
    //extract clock signal name
    g_clock_sig_name = ivl_design_flag(design, "clk");
    if(g_clock_sig_name == NULL){
        error("Clock signal name not given");
    }
    
    //extract reset signal name
    g_reset_sig_name = ivl_design_flag(design, "reset");
    if(g_reset_sig_name == NULL){
        error("Reset signal name not given");
    }
    
    //extract reset edge
    g_reset_edge_active = ivl_design_flag(design, "reset_edge");
    if(g_reset_edge_active == NULL){
        error("Reset edge not given");
    }

}



void generate_sig_file(ivl_scope_t root) {
    const char* name;
    // cout << "Generate sig files " << "\n";
    FILE* f_sig = fopen(g_sig_file, "w");
    const uint sig_count = ivl_scope_sigs(root);

    for(uint i=0; i<sig_count; i++){
        ivl_signal_t sig = ivl_scope_sig(root, i);
        bool fixed = false;
        if (!ivl_signal_local(sig)) {
            name = ivl_signal_basename(sig);
            ivl_nexus_t nex = ivl_signal_nex(sig, 0);
            unsigned idx, count = ivl_nexus_ptrs(nex);
            for (idx = 0; idx < count; idx += 1) {
                ivl_nexus_ptr_t nex_ptr = ivl_nexus_ptr(nex, idx);
                ivl_net_const_t net_const = ivl_nexus_ptr_con(nex_ptr);
                if (!net_const) continue;
                if (root != ivl_const_scope(net_const)) continue;
                fixed = true;
            }
            if (strstr (name,g_clock_sig_name)  == 0 && strcmp(name, g_reset_sig_name) != 0 && !fixed) { //strcmp(name, g_clock_sig_name)
                // cout << "name " << name << "\n";
                fprintf(f_sig, "%s", name);
                fprintf(f_sig, "\n");
        }

        }
       

    }
    fclose(f_sig);



}



void generate_tb(ivl_scope_t root){
    const char* name;
    // cout << "Generate sig files " << "\n";
    //prepare reset edge
    const char *reset_edge_inactive;
    if(strcmp(g_reset_edge_active,"1") == 0){
        g_reset_edge_active = "1'b1";
        reset_edge_inactive = "1'b0";
    }
    else if(strcmp(g_reset_edge_active,"0") == 0){
        g_reset_edge_active = "1'b0";
        reset_edge_inactive = "1'b1";
    }
    else{
        error("Wrong reset edge parameters");
    }

    //write testbench
    FILE* f_tb = fopen(g_tb_file, "w");
    fprintf(f_tb, "module tb();\n\n");
    
    const uint count = ivl_scope_mod_module_ports(root);
    if(count == 0){
        error("Top module do not have any ports");
    }

    bool clk_found = false, reset_found = false;

    fprintf(f_tb, "%*c// Generated top module signals\n", 4, ' ');
    FILE* f_inputs = fopen(g_input_file, "w");
    for(uint i=0; i<count; i++){
        name = ivl_scope_mod_module_port_name(root, i);
        // cout << name << "\n";
        uint temp_width = ivl_scope_mod_module_port_width(root, i);
        const char* type;
        bool is_init_val_req = false;
        if(strcmp(name, g_clock_sig_name) == 0){
            if(temp_width != 1){
                error("Clock signal width is not 1");
            }
            type = "reg ";
            clk_found = true;
        }
        else if(strcmp(name, g_reset_sig_name) == 0){
            if(temp_width != 1){
                error("Reset signal width is not 1");
            }
            type = "reg ";
            reset_found = true;
        } else{
            type = "reg ";
            ivl_signal_port_t port_type = ivl_scope_mod_module_port_type(root, i);
            if(port_type == IVL_SIP_INPUT){
                
                fprintf(f_inputs, "%s\n", name);

				g_data.add_input(name, temp_width);
				is_init_val_req = true;
            } else if(port_type == IVL_SIP_OUTPUT){
                type = "wire";
            }
        }

        if(temp_width == 1){
            fprintf(f_tb, "%*c%s %s", 4, ' ', type, name);
        } else {
            fprintf(f_tb, "%*c%s [%u:0] %s", 4, ' ', type, temp_width - 1, name);
        }

        if(is_init_val_req){
            fprintf(f_tb, " = %d\'b0", temp_width);
        }

        fprintf(f_tb, ";\n");
    }
    fclose(f_inputs);
    // if(enable_obs_padding){
	// 	g_data.add_input("__obs", 1);
    //     fprintf(f_tb, "%*creg  __obs;\n", 4, ' ');
    // }
    if(!clk_found){
        error("Clock signal not found in top module");
    }
    if(!reset_found){
        error("Reset signal not found in top module");
    }

    //Dump top module instantiation
    fprintf(f_tb, "\n%*c// Generated top module instance\n", 4, ' ');
    fprintf(f_tb, "%*c%s _ra_top_inst(\n", 4, ' ', ivl_scope_tname(root));
    if(count){
        name = ivl_scope_mod_module_port_name(root, 0);
        fprintf(f_tb, "%*c.%-10s( %s )", 12, ' ', name, name);
    }
    for(uint i=1; i<count; i++){
        name = ivl_scope_mod_module_port_name(root, i);
        fprintf(f_tb, ",\n%*c.%-10s( %s )", 12, ' ', name, name);
    }
    // if(enable_obs_padding){
    //     fprintf(f_tb, ",\n%*c.__obs     ( __obs )", 12, ' ');
    // }
    fprintf(f_tb, ");\n");

    if(g_data.get_width() >= 0){
        //Dump internal use signals
        fprintf(f_tb, "\n%*c// Generated internal use signals\n", 4, ' ');
        fprintf(f_tb, "%*creg  [31:0] _ra_pc;\n", 4, ' ');
        fprintf(f_tb, "%*creg  [%u:0] _ra_opcode;\n", 4, ' ', g_data.get_width() - 1);
        fprintf(f_tb, "%*creg  [%u:0] _ra_ram[0:%u];\n\n", 4, ' ', g_data.get_width() - 1, g_unroll);

        //Dump clk toggle
        fprintf(f_tb, "\n%*c// Generated clock pulse\n", 4, ' ');
        fprintf(f_tb, "%*calways begin\n", 4, ' ');
        fprintf(f_tb, "%*c#30 %s = ~%s;\n", 8, ' ', g_clock_sig_name, g_clock_sig_name);
        fprintf(f_tb, "%*cend\n", 4, ' ');

        //Dump pc increment
        fprintf(f_tb, "\n%*c// Generated program counter\n", 4, ' ');
        fprintf(f_tb, "%*calways @(posedge %s) begin\n", 4, ' ', g_clock_sig_name);
        fprintf(f_tb, "%*c_ra_opcode = _ra_ram[_ra_pc];\n", 8, ' ');
		
		for(auto it:g_data.in_ports){
			if(it.second->width > 1){
                fprintf(f_tb, "%*c%s <= #5 _ra_opcode[%u:%u];\n", 8, ' ', it.first.c_str(), it.second->msb, it.second->lsb);
            } else {
                fprintf(f_tb, "%*c%s <= #5 _ra_opcode[%u];\n", 8, ' ', it.first.c_str(), it.second->msb);
            }
		}
        fprintf(f_tb, "%*c_ra_pc = _ra_pc + 32'b1;\n", 8, ' ');
        fprintf(f_tb, "%*c$strobe(\";_C %%d\", _ra_pc);\n", 8, ' ');
        fprintf(f_tb, "%*cend\n", 4, ' ');

        //Dump initial block
        fprintf(f_tb, "\n%*c// Generated initial block\n", 4, ' ');
        fprintf(f_tb, "%*cinitial begin\n", 4, ' ');
        // fprintf(f_tb, "%*c$dumpfile(\"sim.vcd\");\n", 8, ' ');
        // fprintf(f_tb, "%*c$dumpvars(0, _ra_top_inst);\n", 8, ' ');
        fprintf(f_tb, "%*c%s = 1'b0;\n", 8, ' ', g_clock_sig_name);
        fprintf(f_tb, "%*c%s = %s;\n", 8, ' ', g_reset_sig_name, reset_edge_inactive);
        fprintf(f_tb, "%*c_ra_pc = 32'b0;\n", 8, ' ');
        fprintf(f_tb, "%*c$readmemb(\"%s\", _ra_ram);\n", 8, ' ', g_data_mem);
        fprintf(f_tb, "%*c#2 %s = 1'b1;\n", 8, ' ', g_clock_sig_name);
        fprintf(f_tb, "%*c%s = %s;\n", 8, ' ', g_reset_sig_name, g_reset_edge_active);
        fprintf(f_tb, "%*c#5 %s = %s;\n", 8, ' ', g_reset_sig_name, reset_edge_inactive);
        fprintf(f_tb, "%*c#%d $finish;\n", 8, ' ', g_unroll * 60);
        fprintf(f_tb, "%*cend\n\n", 4, ' ');
    } else{
        error("TODO: Testbench for input width of 0");
    }

    fprintf(f_tb, "endmodule\n");
    fclose(f_tb);
}

void random_sim() {
    
    g_data.generate();

}

void instrument_design() {
    
}

