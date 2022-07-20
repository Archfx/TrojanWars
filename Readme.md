# Threat heuristics - Malicious Implants Detection Version 2.0


### Quick DEMO

1. Setup the path variables

```shell
source runfirst.sh
```

2. Run the demo benchmark

```shell
cd Benchmarks/demo
source run.sh
```


### How to Run:

This tool is designed as a set of multiple tools that follows a flow. This reduces the process complexity drastically and user has more controlability over the experiments. Please follow the outlined flow in the run.sh file for the first time. 
Each folder contails a tool corresponding to a seperate task in the flow (Example flow is given) of the tool. 

1. Setup the path variables

```shell
source runfirst.sh
```

This will enable the sub tools to be invoked from their name

2. Use the sub tools using the flow

ex : 
```shell
$flatten -d aes.v -t aes -k clk -s rst -c 21
```

To make it simpler, we have included a run.sh file in each benchmark folder, (Note that you can invoke the tool from anywhere as long as the original benchmark is there and the path variables are correclty set)

Example run.sh file from c2670 benchmark is given below, (run this with 'source run.sh')

```shell

export DESIGN=c2670.v
export TOP=c2670  # top module name of the design
export CLK=clk  # clk signal name
export RST=rst  # rst signal name
export INLINE=inline.v
export RARENODES=rare_nodes.txt  # Rare nodes are saved to this file, You can remove or add nodes to here
export CYCLES=3  # Pipe-line deapth or the number of simulation cycles needed
export NCRITERION=100  # Statistical N to actvate each rare node
export RESETEDGE=0  # 0- Rising egde 1-Falling edge

export TESTVECTORS=$TOP\_mid.patterns

export GOLDEN=inline.v
export SUSPECTED=Trojan_in/trojan_inline.v




echo $'\n ****** Flatten Tool ******* \n'
$flatten -f $FILELIST -t $TOP -k $CLK -s $RST -c $CYCLES -x $RESETEDGE -e 10


echo $'\n ****** Rareness Analyzer ******* \n'
$rare -d $INLINE -i 100 -c $CYCLES -r $RARENESS


echo $'\n ****** Instrumented Synthesizer ******* \n'
$synth -t $TOP -d inline.v -r $RARENODES


echo $'\n ****** Statistical Test Generation with ATPG ******* \n'
$ndatpg -t $TOP -d synthesised.v -r $RARENODES -c $CYCLES -n $NCRITERION -k $CLK -s $RST


echo $'\n ****** Statistical Test Generation ******* \n'
$ndetect -d $INLINE -i 1000 -c $CYCLES -t $TOP -n $NCRITERION


echo $'\n ****** Combine generated test vectors to one file ******* \n'
rm $TESTVECTORS 2> /dev/null
cat *.patterns > $TESTVECTORS


echo $'\n ****** Coverage Analysis ******* \n'
$coverage -d $INLINE -r $RARENODES -c $CYCLES -v $TESTVECTORS -n 2 -i 10 -t $TOP


echo $'\n ****** Implant Detection ******* \n'
$midetect -g $GOLDEN -s $SUSPECTED -c $CYCLES -v $TESTVECTORS -o functionaldiff.txt

if [ -s functionaldiff.txt ] # Check for functional difference
then
	 echo $'\n ****** MALICIOUS IMPLANTS DETECTED ******* \n'

else
	 echo $'\n ****** NO MALICIOUS IMPLANTS DETECTED ******* \n'
fi



echo $'\n ****** Cleaning Meta files ******* \n'
rm -r csrc *.daidir 2> /dev/null
rm *.ys *.key *.tcl *.run *.svf 2> /dev/null


```
Note : All the intermediate tools require flattened netlists. When invoking Implant detection, Golden design and the Suspected designs are the flattened versions of the respective designs.

User can change the parameters in the top of the run.sh file according to the benchmark. This way parameters are passed automatically through the tools. To list down commandline arguments you can use -h or --help




### Example Flow:

Flatten Design: 

You can feed multple design files for the flatten tool with seperate file_list -f flags.
```shell
  $flatten -f $FILELIST -t $TOP -k $CLK -s $RST -c $CYCLES
```
Also you can feed multple design files for the flatten tool with seperate -d flags.
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
$ndatpg -t $TOP -d synthesised.v -r $RARENODES -c $CYCLES -n $NCRITERION -k $CLK -s $RST
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

Provided in each benchmark folder with the name of **run.sh**, You can modify top varibles accordingly. Users can invoke the tools using,
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
export CYCLES=3  # Pipe-line deapth or the number of simulation cycles needed
export NCRITERION=100  # Statistical N to actvate each rare node
export RESETEDGE=0  # 0- Rising egde 1-Falling edge

export TESTVECTORS=$TOP\_mid.patterns

export GOLDEN=inline.v
export SUSPECTED=Trojan_in/trojan_inline.v




echo $'\n ****** Flatten Tool ******* \n'
$flatten -f $FILELIST -t $TOP -k $CLK -s $RST -c $CYCLES -x $RESETEDGE -e 10


echo $'\n ****** Rareness Analyzer ******* \n'
$rare -d $INLINE -i 100 -c $CYCLES -r $RARENESS


echo $'\n ****** Instrumented Synthesizer ******* \n'
$synth -t $TOP -d inline.v -r $RARENODES


echo $'\n ****** Statistical Test Generation with ATPG ******* \n'
$ndatpg -t $TOP -d synthesised.v -r $RARENODES -c $CYCLES -n $NCRITERION -k $CLK -s $RST


echo $'\n ****** Statistical Test Generation ******* \n'
$ndetect -d $INLINE -i 1000 -c $CYCLES -t $TOP -n $NCRITERION


echo $'\n ****** Combine generated test vectors to one file ******* \n'
rm $TESTVECTORS 2> /dev/null
cat *.patterns > $TESTVECTORS


echo $'\n ****** Coverage Analysis ******* \n'
$coverage -d $INLINE -r $RARENODES -c $CYCLES -v $TESTVECTORS -n 2 -i 10 -t $TOP


echo $'\n ****** Implant Detection ******* \n'
$midetect -g $GOLDEN -s $SUSPECTED -c $CYCLES -v $TESTVECTORS -o functionaldiff.txt

if [ -s functionaldiff.txt ] # Check for functional difference
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
<li>Python3, Numpy, Psutils, gc, multiprocessing</li>
<li>iGraph library</li>
<li>Yosys</li>
<li>Z3 Theorem Prover</li>
<li>Libraries used by the Z3 Theorem Prover</li>
</ul>

