import sys
import random
import os
import time
import subprocess


from optparse import OptionParser

progressBar_width = 40
nodes = {}



class NODE:
    def __init__(self, name, ref, value, init, iteration):
        self.name = name
        self.ref = ref
        self.value = value
        self.init = init
        self.iteration = iteration


class RARE_NODE:
    def __init__(self, name , bit, value, rareness,bus):
        self.name = name
        self.bit= bit
        self.value = value
        self.rareness = rareness
        self.bus = bus


def count_words_line(line):
    # return len(line.strip().split())
    global nodes

    if (";_C" in line):
        x = line.strip(";_C ")
            # if (int(x) == d):
            #     break
    else:
        x = line.split()
            # print(x[0])
            # print(x[1])
        if (x[0] in nodes and len(x)==2):

            node = nodes.get(x[0])
            # if (node.init == False):
            #     node.init = True
            #     array_size = len(x[1])
            #     node.value = [0]*array_size
                    # print(node.name, node.value)

            if (("x" not in x[1]) and ("z" not in x[1])):
                # node.iteration += 1
                # val = list(map(int, list(x[1])))
                # zipped_lists = zip(val, node.value)
                # node.value = [x + y for (x, y) in zipped_lists]
                node.iteration = 1
                node.value = list(map(int, list(x[1])))
            # print(node.name, node.value, node.iteration)
    return nodes

def progressBar(numerator,denominator,final):
    if not final:
            val = int((numerator/denominator)*progressBar_width)
            progressBar = "["+"-"* val + " "* (progressBar_width - val) +"]" + str(numerator)+"/"+str(denominator) + " "
            sys.stdout.write(progressBar)
            sys.stdout.flush()
            sys.stdout.write("\b" * (len(progressBar)))
    else :
        progressBar = "["+"-"* progressBar_width +"]" + str(numerator)+"/"+str(denominator) + " "
        sys.stdout.write(progressBar)
        sys.stdout.flush()


def main():
    start = time.time()
    # tracemalloc.start()


    print("================================================================================" )
    print("********                  Rareness Analysis Tool                       *********")

    INFO = "Rare-Node Analyzer"
    VERSION = "Version 1.01"
    USAGE = "Usage: python3 rare_node_analysis.py"

    # def showVersion():
    #     print(INFO)
    #     print(VERSION)
    #     print(USAGE)
    #     sys.exit()

    optparser = OptionParser()
    optparser.add_option("-d", "--design", action="store",
                         dest="design_file", help="Design File")
   
    optparser.add_option("-r", "--rare_threshold", action="store",
                         dest="threashold", help="Threshold_Value", default=0.1)
    optparser.add_option("-n", "--number", action="store",
                         dest="no_combination", help="Combination_Value", default=1)
    optparser.add_option("-s", "--skip", action="store",
                         dest="skip", help="Skip random simulation", default=0)
    optparser.add_option("-i", "--iteration", action="store",
                         dest="iteration", help="Skip random simulation", default=1000)
    optparser.add_option("-c", "--cycle", action="store",
                         dest="unroll_cycle", help="Cycles per simulation", default=10)
    optparser.add_option("-v", "--version", action="store_true", dest = "print_version",
                         help="Version of the Tool", default= False)
    (options, args) = optparser.parse_args()

    design_file = options.design_file
    threashold = options.threashold
    combination_values = int(options.no_combination)
    skip = int(options.skip)
    sim_iteration = int(options.iteration)
    unroll_cycle = int(options.unroll_cycle)
    tool = "vcs -q"

    if (options.print_version) : 
        print (VERSION)
        print("================================================================================" )
        exit()

    # if os.path.exists(design_file):
    current_location = '' #os.path.dirname(design_file)

        # print(current_location)

    # File information
    testbench_file = "tb.v"
    data_mem = 'data.mem'
    vvp_file = 'run.sim'
    output_file = 'sim.log'
    rare_file = "rare_nodes.txt"
    node_file = "nodes.txt"
    instrumented_design = "design.v"
    ebmc_design = "ebmc.sv"
    make_file = "Makefile.inc"

    data_filepath = os.path.join(current_location, data_mem)
    vvp_filepath = os.path.join(current_location, vvp_file)
    design_filepath = design_file
    testbench_filepath = os.path.join(current_location, testbench_file)
    output_filepath = os.path.join(current_location, output_file)
    rare_filepath = os.path.join(current_location, rare_file)
    nodes_filepath = os.path.join(current_location, node_file)
    instrumented_filepath = os.path.join(current_location, instrumented_design)
    ebmc_filepath = os.path.join(current_location, ebmc_design)
    make_filepath = os.path.join(current_location, make_file)
    rare_node_filepath = os.path.join(current_location, rare_file)




    input_length = 0

    # # Extract unroll cycle
    # with open(make_filepath, 'r') as fileobj:
    #     for row in fileobj:
    #         if row.__contains__('UNROLL_CYC'):
    #             n = row.rstrip('\n')
    #             key = n.strip('UNROLL_CYC	= ')
    #             unroll_cycle = int(key)




    #Create TB
    # if (skip != 1):
    #     print("Simulation not skipped")
    #     make_command = "make -C "+current_location
    #     os.system(make_command)


    # Read signals
    # nodes = {}
    global nodes
    with open(nodes_filepath, 'r') as fileobj:
        tik = 0
        for row in fileobj:
            n = row.rstrip('\n')
            # print(n)
            nodes[tik] = NODE(n, tik, [], False, 0)
            tik = tik + 1
            

    if (skip != 1):
        # Instrument the design
        sed_command = "sed 's/\<endmodule\>//g' "+design_file +" > "+ instrumented_filepath
        # print(sed_command)
        os.system(sed_command)

        # Replace path
        # replace_command = "sed -i 's#data.mem#"+data_filepath+"#g' " +testbench_filepath
        # print(replace_command)
        # os.system(replace_command)

        # print(nodes.get(0).name)
        with open(instrumented_filepath, "a") as ins_file:
            for ref, val in nodes.items():
                # if (not "_s" in node and not "clk" in node ):
                if ( not "clk" in nodes.get(ref).name ):
                    
                    if ("." in nodes.get(ref).name or "[" in nodes.get(ref).name):
                        ins_file.write("always @ (posedge \\"+val.name+" or negedge \\"+val.name+" ) begin \n")
                        s = '\t$display("'+str(ref)+' %b", \\' +val.name+' );\n'
                    else:
                        ins_file.write("always @ (posedge "+val.name+" or negedge "+val.name+" ) begin \n")
                        s = '\t$display("'+str(ref)+' %b", ' +val.name+ ' );\n'
                    ins_file.write(s)
                    ins_file.write("end\n")

            ins_file.write("endmodule\n")

    # Command information
    compile_command = tool +' -o '  + vvp_filepath + " " + instrumented_filepath + " " + testbench_filepath
    # print(compile_command)
    if (tool == "vcs -q") : run_command = './'+vvp_filepath + ' -q ' + " > " + output_filepath
    else : run_command = 'vvp '+vvp_filepath + " > " + output_filepath
    # print(run_command)

    # complie and run the testbench for first time
    # os.system(compile_command)
    # os.system(run_command)
    process = subprocess.Popen(compile_command, shell=True, stdout=subprocess.PIPE)
    process.wait()

    process = subprocess.Popen(run_command, shell=True, stdout=subprocess.PIPE)
    process.wait()

    # Extracting data length from data_mem
    with open(data_filepath, 'r') as fileobj:
        for row in fileobj:
            n = row.rstrip('\n')
            input_length = len(n)
            break

    print("Design input length", input_length)
    # Generate random test and simulate 10000 times
    print("Simulation Started ")

    # print(nodes)
    # for x in nodes:
    #     print(x.value)

    for simulation_iteration in range(1,sim_iteration):
        with open(data_filepath, "w") as fileobj:

            for i in range(0, unroll_cycle+1):
                n = 2**input_length
                a = random.randint(0, n-1)
                binary_string = bin(a)[2:].zfill(input_length)
                fileobj.write(binary_string)
                fileobj.write("\n")
            
        
        
        # os.system(compile_command)
        # os.system(run_command)
        process = subprocess.Popen(run_command, shell=True, stdout=subprocess.PIPE)
        process.wait()
        # print(simulation_iteration, run_command)

        with open(output_filepath) as fileobject:
            for line in fileobject:
                if (";_C" in line):
                    x = line.strip(";_C ")
                else:
                    x = line.split()
                    if(len(x)==2 and x[0].isdigit()):
                        if (int(x[0]) in nodes):
                            node = nodes.get(int(x[0]))
                            if (node.init == False):
                                node.init = True
                                array_size = len(x[1])
                                node.value = [0]*array_size
                            # print(node.name, node.value)

                            if (("x" not in x[1]) and ("z" not in x[1]) and x[1].isnumeric()):
                                node.iteration += 1
                                #print(list(x[1]))
                                val = list(map(int, list(x[1].strip())))
                                # print(val)
                                zipped_lists = zip(val, node.value)
                                node.value = [x + y for (x, y) in zipped_lists]
        progressBar(simulation_iteration,sim_iteration,False)
    progressBar(sim_iteration,sim_iteration,True)    
 
    sys.stdout.write("\n")


    rare_nodes = []
    minimum_rareness = 1.0
    for node , val in  nodes.items():
        
        # print(node)
        # print(val.iteration)
        bit = 0
        iteration = val.iteration
        if len(val.value) == 1 : bus = False
        else : bus = True
        for bit_value in val.value:
            if (not iteration == 0):
                rareness = bit_value/iteration
                if ((rareness < minimum_rareness) and rareness > 0):
                        minimum_rareness=rareness
                if ((1-rareness) < minimum_rareness and (1-rareness) > 0):
                        minimum_rareness=1-rareness
              
                if (rareness <= float(threashold)):
                    rare_nodes.append(RARE_NODE(val.name, bit, 1, rareness,bus))
                    
                if ((1-rareness) <= float(threashold)):
                    rare_nodes.append(RARE_NODE(val.name, bit, 0, 1-rareness,bus))
                   

                # print(node,"[",bit, "] : ", rareness)
            bit +=1

    print("RARE NODES saved to "+ rare_node_filepath )
    # for n in range(combination_values):
    #     data = [0]*(n+1); 
    #     rare_combination(rare_nodes, data, assertions, 0, len(rare_nodes)-1, 0, n+1)

    rare_nodes = sorted(rare_nodes, key=lambda x: x.rareness)

    #for node in rare_nodes:
        # print(node.name, ":", node.value)
     #   if node.bus :
      #      print(node.name,"[",node.bit, "] value of ",node.value, ": ", node.rareness)
       # else :
        #    print(node.name," value of ",node.value, ": ", node.rareness)

    with open(rare_node_filepath, "w") as f:
        f.write("Rare Nodes in the design\n")
        for n in rare_nodes:
            if n.bus :
                if ("[" not in n.name): f.write(f"{n.name}[{n.bit}]: {n.value}  : {n.rareness}" +"\n")
            else :
                f.write(f"{n.name}: {n.value}  : {n.rareness}" +"\n")


    print("**********RARENESS INFO**********")
    print("Minimum non zero rareness : ", minimum_rareness)
    end = time.time()
    print("Rareness threshold : ", threashold)
    print("Simulation Iteration : ", sim_iteration)
    print ("Design : ",design_file)


        
if __name__ == '__main__':
    try:
        main()
    except TypeError:
        print("Invalid arguements, please use -h flag for help")
    except FileNotFoundError:
        print("Invalid arguements, please use -h flag for help")
    print("================================================================================" )
