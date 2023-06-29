# TrojanWars - Hardware Malicious Implants Detection Tool

Hardware trojan detection is a critical process that aims to identify and mitigate the presence of malicious alterations or additions in integrated circuits or electronic devices. It involves various techniques and methodologies to ensure the integrity and trustworthiness of hardware components. Hardware trojans can be inserted during the manufacturing process, and they can pose significant risks by compromising the functionality, security, and reliability of the affected systems. Detection methods range from physical inspections, such as side-channel analysis and optical inspection, to more advanced techniques like hardware fingerprinting, anomaly detection, and formal verification. By employing a combination of these approaches, hardware trojan detection plays a vital role in safeguarding critical infrastructure, preventing unauthorized access, and maintaining the trustworthiness of electronic systems. TrojanWars utilizes efficient test generation-based techniques to activate malicious implants if present in the designs.

### DEMO

1. Setup the path variables

```shell
source runfirst.sh
```

2. Go to the Benchmark directory

```shell
cd Benchmarks/demo
```

3. Run the demo benchmark

```shell
source run.sh
```

### UPDATES in Alpha 2

New directed test generation technique MCATPG added. Add the below line to **run.sh** script

```shell

echo $'\n ****** Directed Test Generation with ATPG ******* \n'
$mcatpg -t $TOP -r $RARENODES -c $CYCLES -k $CLK -s $RST -g $GRAPHSIZE

```



### NOTES:

1. The Benchmarks directory includes 10 different types of Trojan instances (each benchmark covers one type of Trojan instance).  

2. To run any of these benchmarks, use the same processes above except the second step should point to the correct directory (Ex: cd Benchmarks/RS232_Trojan_Sequence_Of_Rare_Events/ ).

3. The script "run.sh" performs four major tasks: (i) rareness analysis, (ii) test generation using two methods, (iii) coverage analysis using randomly sampled Trojans, and (iv) detection of hardware Trojans for a given implementation.

4. The script "run.sh" has many parameters that can be tuned for improved detection. For example, the NCRITERION is set to 10 for faster completion, however, it can be increased to improve the probability of Trojan detection.  


### Example Flow:

Setup the path variables:

```shell
source runfirst.sh
```

Flatten Design: 

You can feed multiple design files for the flatten tool with separate file_list -f flags.
```shell
  $flatten -f $FILELIST -t $TOP -k $CLK -s $RST -c $CYCLES
```
Also, you can feed multiple design files for the flatten tool with separate -d flags.
```shell
  $flatten -d $DESIGN1 -d $DESIGN2 -d $DESIGN3 -t $TOP -k $CLK -s $RST -c $CYCLES
```

Rareness Analysis: 
```shell
  $rare -d $INLINE -i 1000 -c $CYCLES
```

Test Generation: (we provide two methods for test generation)
```shell
$ndetect -d $INLINE -i 1000 -c $CYCLES -t $TOP -n $NCRITERION

$synth -t $TOP -d inline.v -r $RARENODES
$ndatpg -t $TOP -r $RARENODES -c $CYCLES -n $NCRITERION -k $CLK -s $RST
$mcatpg -t $TOP -r $RARENODES -c $CYCLES -k $CLK -s $RST -g $GRAPHSIZE
```

Coverage Analysis: 
```shell
$coverage -d $INLINE -r $RARE_NODE_FILE -c $CYCLES -v c2670_ndatpg.patterns -n 2 -i 10 -t $TOP
```

Implant Detection: 
```shell
$midetect -g $GOLDEN -s $SUSPECTED -v c2670_ndetect.patterns -o functionaldiff.txt
```

Additionally, we will provide .tcl scripts to automate the entire flow. This flow minimizes the redundancy of computations for optimal and fast results.

### Example .sh script 

Provided in each benchmark folder with the name of **run.sh**, You can modify top variables accordingly. Users can invoke the tools using,
```shell
source run.sh

```

Example **run.sh** for c2670.v benchmark
```shell

export DESIGN=c2670.v
export TOP=c2670  # top module name of the design
export CLK=clk  # clk signal name
export RST=rst  # rst signal name
export INLINE=inline.v
export RARENODES=rare_nodes.txt  # Rare nodes are saved to this file, You can remove or add nodes to here
export CYCLES=3  # Pipe-line depth or the number of simulation cycles needed
export RESETEDGE=0  # 0- Rising edge 1-Falling edge

export NCRITERION=100 # Statistical N to actvate each rare node
export GRAPHSIZE=75 # Satisfiability graph size (number of edges) for graph created from rare signals

export TESTVECTORS=$TOP\_mid.patterns

export GOLDEN=inline.v
export SUSPECTED=Trojan_in/trojan_inline.v




echo $'\n ****** Flatten Tool ******* \n'
$flatten -f $FILELIST -t $TOP -k $CLK -s $RST -c $CYCLES -x $RESETEDGE -e 10


echo $'\n ****** Rareness Analyzer ******* \n'
$rare -d $INLINE -i 100 -c $CYCLES -r $RARENESS


echo $'\n ****** Instrumented Synthesizer ******* \n'
$synth -t $TOP -d inline.v -r $RARENODES


echo $'\n ****** Directed Test Generation with ATPG ******* \n'
$mcatpg -t $TOP -r $RARENODES -c $CYCLES -k $CLK -s $RST -g $GRAPHSIZE


echo $'\n ****** Statistical Test Generation with ATPG ******* \n'
$ndatpg -t $TOP -r $RARENODES -c $CYCLES -n $NCRITERION -k $CLK -s $RST


echo $'\n ****** Statistical Test Generation ******* \n'
$ndetect -d $INLINE -i 1000 -c $CYCLES -t $TOP -n $NCRITERION


echo $'\n ****** Combine generated test vectors to one file ******* \n'
rm $TESTVECTORS 2> /dev/null
cat *.patterns > $TESTVECTORS


echo $'\n ****** Coverage Analysis ******* \n'
$coverage -d $INLINE -r $RARENODES -c $CYCLES -v $TESTVECTORS -n 2 -i 10 -t $TOP


echo $'\n ****** Implant Detection ******* \n'
$midetect -g $GOLDEN -s $SUSPECTED -c $CYCLES -v $TESTVECTORS -o functionaldiff.txt

if [ -s functionaldiff.txt ] # Check for the functional difference
then
	 echo $'\n ****** MALICIOUS IMPLANTS DETECTED ******* \n'

else
	 echo $'\n ****** NO MALICIOUS IMPLANTS DETECTED ******* \n'
fi



echo $'\n ****** Cleaning Meta files ******* \n'
rm -r csrc *.daidir 2> /dev/null
rm *.ys *.key *.tcl *.run *.svf 2> /dev/null


```

### Required libraries and environment

The current version of the tool will utilize the following libraries and packages.
<ul>
<li>VCS/Icarus Verilog</li>
<li>Synopsys Tmax, DC_shell</li>
<li>C++ 11</li>
<li>Python3</li>
<li>Yosys</li>
</ul>

### Cite this work

*A. Jayasena and P. Mishra, "Scalable Detection of Hardware Trojans using ATPG-based Activation of Rare Events," in IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, doi: 10.1109/TCAD.2023.3290537.*

```latex
@ARTICLE{Jayasena2023Scalable,
  author={Jayasena, Aruna and Mishra, Prabhat},
  journal={IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems}, 
  title={Scalable Detection of Hardware Trojans using ATPG-based Activation of Rare Events}, 
  year={2023},
  volume={},
  number={},
  pages={1-1},
  doi={10.1109/TCAD.2023.3290537}}
```
