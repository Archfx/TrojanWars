import random
import subprocess
import sys
from optparse import OptionParser

N = 5 # Trigger Size
nT = 100 # Number of Triggers
nT_list =[]

tL = [] #Trigger list

unroll_cycle = 1
progressBar_width = 40
top = ''



def trig2File(strList):
    w = open("trigger.txt", "w")
    for x in strList:
        w.write(x+"\n")
    w.close()

def alwaysvblock(strList):
    block = '''
always @(*) begin
'''

    for x in range(len(strList)):
        block = block + '''
        if (''' + strList[x] + ''') begin
            $display(" ASN : '''+str(x)+''' ");
        end
        '''
    block = block + '''    
end'''
#     block = ''


#     for x in range(len(strList)):
#         block = block + '''
# always @(posedge (''' + strList[x] + ''')) begin 
#         $display("ASN : '''+str(x)+''' :");
# end
#         '''
    # print(block)
    return block

def verify(nodes,top):

    constraints = ''
    stuck = ''
    # condition = ''
    for y in nodes:
        if ("[" in y[0] and "]" in y[0]):
            nodename = (y[0].replace("[","_")).replace("]","_")
        else :  nodename = y[0]       
        constraints = constraints + "add_atpg_constraints a "+y[1] + " -module "+ top + " "+nodename + " "+ "\n"
        
        if (y[1].strip()=='0'):
            stuck = stuck + "add_faults  -stuck  1 " + nodename + "\n"
        if (y[1].strip()=='1'):
            stuck = stuck + "add_faults  -stuck 0 " + nodename + "\n"
    # condition = "~(" + condition[3:] + "))"#+ "==0"
    # # print(condition)
    
    # print(ebmc)
    # os.system(ebmc)
    # proc = subprocess.Popen(ebmc, stdout=subprocess.PIPE, shell=True)

        tclContent = """set_messages -nodisplay
set_commands noabort
read_image design.image
set_faults -model stuck
""" + constraints +"""
""" + stuck +"""
set_atpg -merge high -full_seq_time {60 240} -full_seq_atpg
run_atpg
set_messages -display
report_patterns -all -internal
#write_patterns $testPatterns -format stil -replace
exit"""

    w = open("query.tcl", "w")
    w.write(tclContent)
    w.close()

    out = subprocess.check_output("tmax -shell -tcl query.tcl", shell=True).decode("utf-8") 
    #print(out)
    if " Pattern 0" in out:
        # print ("SAT")
        return True
    else : return False


def list2str(ls):
    strList = []
    for x in ls:
        condition = ''
        for y in x:
            if (y[1].strip()=='0'):
                condition = condition + " && ~" + y[0] + " "
            if (y[1].strip()=='1'):
                condition = condition + " && " + y[0] + " "
        condition = condition[3:]
        strList.append(condition)
        # print (condition)
    return strList


def extractVectors(vectors):
    w = open("data.mem", "w")
    for x in vectors:
        w.write(x)
    w.close()

def evaluate():
    oUt = open("output.txt", "r")
    lines = oUt.readlines()

    for x in lines:
        if ":" in x and "ASN" in x:
            try:
                if (int(x.split(":")[1].strip()))< len(nT_list):
                    nT_list[int(x.split(":")[1].strip())] = 1
                #else : print ((int(x.split(":")[1].strip())))
            except ValueError:
                print("Skipped")

    oUt.close()

def coverage():
    if (len(nT_list) == 0) : Coverage = float(1)
    else : Coverage = float(sum(nT_list)/len(nT_list))
    # print(nT_list)
    # print ("Coverage :", Coverage)
    return Coverage

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


INFO = "Rare-Node Analyzer"
VERSION = 2.0
USAGE = "Usage: python3 rare_node_analysis.py"


# def showVersion():
#     print(INFO)
#     print(VERSION)
#     print(USAGE)
#     sys.exit()

def main():

    
    print("================================================================================" )
    print("*********                  Coverage Analysis Tool                       ********")

    optparser = OptionParser()
    optparser.add_option("-d", "--design", action="store",
                            dest="design_file", help="Design File", default="inline.v")

    optparser.add_option("-r", "--rarenodes", action="store",
                            dest="rarenodes", help="Threshold_Value", default="rare_nodes.txt")
    optparser.add_option("-n", "--number", action="store",
                            dest="no_combination", help="Trigger size", default=1)
    optparser.add_option("-i", "--iteration", action="store",
                            dest="iteration", help="Number of Trojans", default=10)
    optparser.add_option("-c", "--cycle", action="store",
                            dest="unroll_cycle", help="Cycles per simulation", default=10)
    optparser.add_option("-t", "--top", action="store",
                            dest="top", help="Top module of the design under test")
    optparser.add_option("-v", "--vectors", action="store",
                            dest="patterns", help="Tool used for the simulation")
    (options, args) = optparser.parse_args()

    design_file = options.design_file
    unroll_cycle = int(options.unroll_cycle)
    N = int(options.no_combination)
    nT = int(options.iteration)
    rarenodes_file = options.rarenodes
    patterns = options.patterns
    top = options.top

    if N == 1 : N=2

    design = open(design_file, "r")
    designCov = open("coverage.v", "w")
    

    my_file = open(rarenodes_file, "r")
    content = my_file.read().strip()
    content_list = content.split("\n")

    content_list = content_list[1:]
    # print(content_list)
    lRare = len(content_list)
    counter = 0
    for x in range(nT*10):
        rN = random.randrange(1,N) #Random number of Triggers
        tl = []
        for y in range(rN):
            tl.append(content_list[random.randrange(lRare)].split(": ")[0:2])
        # print(tl)
        # verify(tl,top)
        if (verify(tl,top)):
            # print(tl)
            tL.append(tl)
            counter+=1
        
        # tL.append(tl)
        counter+=1

        if counter == nT:
            break
    # print(tL)

    lines = design.readlines()

    nT_list = [0]*len(tL)




    strList = list2str(tL)
    # print(strList)
    code = alwaysvblock(strList)
    trig2File(strList)
    # print(code)
    for x in lines:
        # print(x)
        if "endmodule" in x :
            designCov.write(code)
            designCov.write ("\n endmodule \n")
        else :
            designCov.write(x)

    designCov.close()
    print ("\nCoverage design instrumented")


    allVectors = open(patterns, "r")
    vectors = allVectors.readlines()


    compile_command = 'vcs -q -o run.sim coverage.v tb.v'
    run_command =  './run.sim -q > output.txt'


    # complie and run the testbench for first time
    process = subprocess.check_output(compile_command, shell=True)
    # process.wait()

    print ("Simulating with Generated Test Vectors")

    cov = open("Coverage.txt", "a")
    # cov.write("test,coverage\n")
    cov.write("===================================================================================\n" )
    cov.write("Test Pattern File : "+ patterns + "\n")

    for x in range (int(len(vectors)/unroll_cycle)):
        # print("new vector")
        vector = vectors[x:x+unroll_cycle]
        # print(vector)
        extractVectors(vector)
        process = subprocess.check_output(run_command, shell=True)
        # process.wait()
        evaluate()
        # cov.write(str(x+1)+","+coverage()+"\n")
        progressBar(x,int(len(vectors)/unroll_cycle),False)
    progressBar(int(len(vectors)/unroll_cycle),int(len(vectors)/unroll_cycle),True) 

    cov.write("Number of Valid Trojans : "+ str(len(tL))+ "\n")
    cov.write("Valid trojan sample : \n")
    for x in tL:
        cov.write( str(x)+ "\n")

    cov.write("Number of test iterations : "+ str(int(len(vectors)/unroll_cycle))+ "\n")
    cov.write("Coverage : "+ str(coverage()*100)+"%"+ "\n")

    print("\n")
    

    print("\n+-------------------------------------------+")
    print("|                  Results                  |")
    print("+-------------------------------------------+")
 	
    MIC = coverage()            
    if (MIC < 0.5): 
        print("| Malicious Implant Coverage (MIC) : %.2f" %MIC+"   |")
        print("| Security Level : 1 (LOW)                  |")
        cov.write(f"Malicious Implant Coverage (MIC) : {MIC:.2f} \n")
        cov.write("Security Level : 1 (LOW)\n")
    elif (MIC < 0.9 and MIC >= 0.5): 
        print("| Malicious Implant Coverage (MIC) : %.2f" %MIC+"   |")
        print("| Security Level : 3 (MEDIUM)               |")
        cov.write(f"Malicious Implant Coverage (MIC) : {MIC:.2f} \n")
        cov.write("Security Level : 3 (MEDIUM)\n")
    elif (MIC >= 0.9): 
        print("| Malicious Implant Coverage (MIC) : %.2f" %MIC+"   |")
        print("| Security Level : 5 (HIGH)                 |")  
        cov.write(f"Malicious Implant Coverage (MIC) : {MIC:.2f} \n")
        cov.write("Security Level : 5 (HIGH)\n")  

    print("+-------------------------------------------+")
    print()

    cov.write("===================================================================================\n" )
    cov.close()

    print("Coverage Analysis done; results saved to Coverage.txt")


if __name__ == '__main__':
    try:
        main()
    except TypeError:
        print("Invalid arguements, please use -h flag for help")
    except FileNotFoundError:
        print("Invalid arguements, please use -h flag for help")
    print("================================================================================" )