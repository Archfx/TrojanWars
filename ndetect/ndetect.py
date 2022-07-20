from builtins import print
import sys
import random
import os
import time
# import tracemalloc
# import multiprocessing as mp
import gc
import subprocess


from optparse import OptionParser
# from mp import Pool
progressBar_width = 40
rarenodes = {}



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
    else:
        x = line.split()

        if (x[0] in nodes):

            node = nodes.get(x[0])
            if (("x" not in x[1]) and ("z" not in x[1])):
                node.iteration = 1
                node.value = list(map(int, list(x[1])))
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



    print("================================================================================" )
    print( "\nNDETECT : Activate rare nodes with statistical Ndetect criterion tool")


    INFO = "Rare-Node Analyzer"
    VERSION = 2.0
    USAGE = "Usage: python3 rare_node_analysis.py"


    optparser = OptionParser()
    optparser.add_option("-d", "--design", action="store",
                         dest="design_file", help="Design File")
    optparser.add_option("-t", "--top", action="store",
                         dest="top_module", help="top_module")
    optparser.add_option("-i", "--iteration", action="store",
                         dest="iteration", help="Skip random simulation", default=1000)
    optparser.add_option("-c", "--cycle", action="store",
                         dest="unroll_cycle", help="Cycles per simulation", default=10)
    optparser.add_option("-n", "--ndetect", action="store",
                         dest="ndetect", help="Statistical Ndetect criterion", default=100)
    optparser.add_option("-v", "--sim", action="store",
                         dest="tool", help="Tool used for the simulation", default="vcs -q")
    (options, args) = optparser.parse_args()

    design_file = options.design_file
    top_module = options.top_module
    sim_iteration = int(options.iteration)
    unroll_cycle = int(options.unroll_cycle)
    tool = options.tool
    ndetect = int(options.ndetect)



    current_location = '' 
    testbench_file = "tb.v"
    data_mem = 'data.mem'
    vvp_file = 'run.sim'
    output_file = 'sim.log'
    rare_file = "rare_nodes.txt"
    instrumented_design = "design_ndetect.v"
    test_pattern_file = top_module + '_ndetect.patterns'
    log_file = 'mid.log'

    data_filepath = os.path.join(current_location, data_mem)
    vvp_filepath = os.path.join(current_location, vvp_file)
    testbench_filepath = os.path.join(current_location, testbench_file)
    output_filepath = os.path.join(current_location, output_file)
    instrumented_filepath = os.path.join(current_location, instrumented_design)
    rare_node_filepath = os.path.join(current_location, rare_file)

    input_length = 0

    global rarenodes
    with open(rare_node_filepath, 'r') as fileobj:
        tik = 0
        for row in fileobj:
            if ":" in row:
                n = row.rstrip('\n')
                bus = False
                bit = 0
                if "[" in n and "]" in n:
                    bus = True
                    #print((n.split("[")[1]).split("]"))
                    bit = int((n.split("[")[1]).split("]")[0])
                n = n.split(":")
                rarenodes[tik] = RARE_NODE(n[0], bit, int(n[1].strip()), n[2].strip(),bus)                
                tik = tik + 1
        if (tik==0):
            print ("No rare nodes found, Please re-run the rareanalyser with different rareness threshold")
            sys.exit(0)                    


    # Instrument the design
    sed_command = "sed 's/\<endmodule\>//g' "+design_file +" > "+ instrumented_filepath
    os.system(sed_command)
    with open(instrumented_filepath, "a") as ins_file:
        for ref, val in rarenodes.items():
            if ( not "clk" in rarenodes.get(ref).name ):
                if ("." in rarenodes.get(ref).name): # or "[" in rarenodes.get(ref).name):
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
    # # print(compile_command)
    if (tool == "vcs -q") : run_command = './'+vvp_filepath + ' -q ' + " > " + output_filepath
    else : run_command = 'vvp '+vvp_filepath + " > " + output_filepath
    # # print(run_command)


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

    print("Simulation Started \n")

    activated = False
    iterCount = 0

    for simulation_iteration in range(1,sim_iteration):
        test_vector = []
        with open(data_filepath, "w") as fileobj:            
            for i in range(0, unroll_cycle+1):
                n = 2**input_length
                a = random.randint(0, n-1)
                binary_string = bin(a)[2:].zfill(input_length)
                fileobj.write(binary_string)
                fileobj.write("\n")
                test_vector.append(a)

        process = subprocess.Popen(run_command, shell=True, stdout=subprocess.PIPE)
        process.wait()


        with open(output_filepath) as fileobject:
            for line in fileobject:
                if (";_C" in line):
                    # x = line.strip(";_C ")
                    pass
                else:
                    x = line.split()
                    if(len(x)==2 and x[0].isdigit() and x[1].isdigit()):
                        if (int(x[0]) in rarenodes):
                            # print(rarenodes)
                            if (rarenodes[int(x[0])].value == int(x[1])):
                                # print(rarenodes[int(x[0])].name, ":", rarenodes[int(x[0])].value)
                                activated = True
                                break
        if activated :
            activated = False                    
            with open(test_pattern_file, "a") as pattern_file:
                for vector in test_vector:
                    pattern_file.write(bin(vector)[2:].zfill(input_length))
                    pattern_file.write("\n")

            
            # with open(data_filepath, "w") as datamem:
            for x in range (1,ndetect+1):
                # for i in range(0, unroll_cycle+1):
                # for vector in test_vector:
                with open(data_filepath, "w") as datamem:
                    for vector in test_vector:
                    # n = 2**input_length
                        if (x< input_length) :
                            binary_string = bin(vector+2**x)[2:].zfill(input_length)
                        else :
                            binary_string = bin(vector+x)[2:].zfill(input_length)
                        datamem.write(binary_string)
                        datamem.write("\n")

                process = subprocess.Popen(run_command, shell=True, stdout=subprocess.PIPE)
                process.wait()

                with open(output_filepath) as simlog:
                    for simline in simlog:
                        if (";_C" in line):
                            pass
                            # y = simline.strip(";_C ")
                        else:
                            y = simline.split()
                            # print(y)
                            if(len(y) == 2 and y[0].isdigit() and y[1].isdigit()):
                                if (int(y[0]) in rarenodes):
                                    # print ("A rare node activated : 2")
                                    if (rarenodes[int(y[0])].value == int(y[1])):
                                        # print ("A rare node activated : 3")
                                        with open(test_pattern_file, "a") as pattern_file:
                                            for vector in test_vector:
                                                if (x< input_length) :
                                                    binary_string = bin(vector+2**x)[2:].zfill(input_length)
                                                else :
                                                    binary_string = bin(vector+x)[2:].zfill(input_length)
                iterCount = iterCount + 1

                progressBar(iterCount,sim_iteration*ndetect,False)
        else : iterCount = iterCount + ndetect

        progressBar(simulation_iteration*ndetect,sim_iteration*ndetect,False)
    progressBar(sim_iteration*ndetect,sim_iteration*ndetect,True)    
 
    sys.stdout.write("\n")

    print("N-Detect Generated Test Vectors saved to " + test_pattern_file )
    print("")
    

if __name__ == '__main__':
    try:
        main()
    except TypeError:
        print("Invalid arguements, please use -h flag for help")
    except FileNotFoundError:
        print("Invalid arguements, please use -h flag for help")
    print("================================================================================" )