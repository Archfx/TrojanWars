import os
from optparse import OptionParser
import os
from optparse import OptionParser
import re
import subprocess
import sys

progressBar_width = 40

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
    optparser = OptionParser()
    optparser.add_option("-g", "--golden_file", action="store", dest="golden_file", help="Golden Design File")
    optparser.add_option("-s", "--suspected_file", action="store", dest="suspected_file", help="Design File with suspected trojans")
    optparser.add_option("-v", "--test_vector", action="store", dest="test_vector", help = "Trojan trigger vectors")
    optparser.add_option("-o", "--diff", action="store", dest="diff", help = "File to save XOR results of two designs", default = "diff.txt")
    optparser.add_option("-c", "--cycle", action="store", dest="unroll_cycle", help="Cycles per simulation", default=10)
    (options, args) = optparser.parse_args()

    TESTBENCH_FILE = 'tb.v'
    DATA_MEM = 'data.mem'

    cycles = int(options.unroll_cycle)
    golden_file = options.golden_file
    suspected_file = options.suspected_file
    test_vector = options.test_vector
    diff = options.diff
    # Initialize File paths
    if os.path.exists(golden_file):
        current_location = os.path.dirname(golden_file)
    else:
        raise FileNotFoundError(f'Design file {golden_file} not found')

    golden_filepath =  golden_file
    suspected_filepath =  suspected_file
    testvector_filepath =  test_vector
    goldentb_filepath =  "midetect_tb.v"
    # suspectedtb_filepath =  "suspected_tb.v"
    goldenvvp_filepath =  "gold_sim.run"
    suspectedvvp_filepath =  "suspect_sim.run"
    goldenoutput_filepath =  "golden_output.txt"
    suspectedoutput_filepath =  "suspected_output.txt"
    testbench_filepath =  TESTBENCH_FILE
    data_filepath =  DATA_MEM
    diff_file =  diff


    print("Instrumenting testbench ")
    output_nodes = []
    with open(testbench_filepath, 'r') as fileobj:
        for row in fileobj:
            n = row.rstrip('\n')
            if "wire" in n:
                output_nodes.append(n.split()[-1][:-1])
            if "_ra_opcode;" and "reg" in n:
                input_tv_len = (re.split('[ | ]',n.strip())[2]).split(':')[0][1:]
    node_string = '        $strobe(";_C %d", _ra_pc);'
    for x in output_nodes:
        node_string = node_string + '\n        $display("'+x+' %b",'+x+');'
    
    input_tv_len = int(input_tv_len) +1

    # Edit the testBench

    with open(testbench_filepath, 'r') as fileobj:
        with open(goldentb_filepath, "w") as f:
            for row in fileobj:
                n = row.rstrip('\n')
                if '$strobe(";_C %d", _ra_pc);' in n:
                    f.write(node_string+"\n")
                else: f.write(row)


    testvector_filepath =  test_vector

    # Compile design
    compile_command = 'vcs -q -o ' + goldenvvp_filepath + " " + golden_file + " " + goldentb_filepath
    process = subprocess.Popen(compile_command, shell=True, stdout=subprocess.PIPE)
    process.wait()

    compile_command = 'vcs -q -o ' + suspectedvvp_filepath + " " + suspected_file + " " + goldentb_filepath
    #print(compile_command)
    process = subprocess.Popen(compile_command, shell=True, stdout=subprocess.PIPE)
    process.wait()

    # Open Data.mem file and edit
    # Open Generated test files

    print("Simulation Started : ")

    simulation_counter = 0
    with open(testvector_filepath, 'r') as fileobj:
        numVectors = len(fileobj.readlines())
    with open(testvector_filepath, 'r') as fileobj:
        for row in fileobj:
            n = row.rstrip('\n')
            #print(n)
            simulation_counter+=1
            #print("Simulating for the Test Vector "+str(simulation_counter)+ " for " + str(len(n)/input_tv_len) + " Cycles")
            f = open(data_filepath , "a")
            #for x in range (cycles):
            f.write(n+"\n")
            
            
            if (simulation_counter%cycles == 0):
                f.close()
                run_command = './'+ goldenvvp_filepath + " -q > " + goldenoutput_filepath
                process = subprocess.Popen(run_command, shell=True, stdout=subprocess.PIPE)
                process.wait()
                #print(run_command)
                run_command = './'+ suspectedvvp_filepath + " -q > " + suspectedoutput_filepath
                process = subprocess.Popen(run_command, shell=True, stdout=subprocess.PIPE)
                process.wait()

                diff_command = "diff -u "+ goldenoutput_filepath +" "+ suspectedoutput_filepath +" >> "+ diff_file
                process = subprocess.Popen(diff_command, shell=True, stdout=subprocess.PIPE)
                process.wait()
				
                f = open(data_filepath , "w")
                f.write("")
                f.close()

            progressBar(simulation_counter,numVectors,False)
    progressBar(numVectors,numVectors,True)
    print()
    print("Output saved to "+ diff_file)
    print() 

  
    return

if __name__ == '__main__':
    print("================================================================================" )
    print("*********                    Implant Detect Tool                        ********")
    try:
        main()
    except TypeError:
        print("Invalid arguements, please use -h flag for help")
    except FileNotFoundError:
        print("Invalid arguements, please use -h flag for help")
    print("================================================================================" )