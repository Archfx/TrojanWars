export DESIGN=counter.v
export TOP=up_counter
export CLK=clk
export RST=reset
export INLINE=inline.v
export RARENODES=rare_nodes.txt
export CYCLES=10
export NCRITERION=4
export RARENESS=0.4
export RESETEDGE=1

export TESTVECTORS=$TOP\_mid.patterns

export GOLDEN=inline.v
export SUSPECTED=Trojan_in/counter.v




echo $'\n ****** Flatten Tool ******* \n'
$flatten -d $DESIGN -t $TOP -k $CLK -s $RST -c $CYCLES -x $RESETEDGE


echo $'\n ****** Rareness Analyzer ******* \n'
$rare -d $INLINE -i 20 -c $CYCLES -r $RARENESS


echo $'\n ****** Instrumented Synthesizer ******* \n'
$synth -t $TOP -d inline.v -r $RARENODES


echo $'\n ****** Statistical Test Generation with ATPG ******* \n'
$ndatpg -t $TOP -d synthesised.v -r $RARENODES -c $CYCLES -n $NCRITERION -k $CLK -s $RST


echo $'\n ****** Statistical Test Generation ******* \n'
$ndetect -d $INLINE -i 10 -c $CYCLES -t $TOP -n $NCRITERION


echo $'\n ****** Combine generated test vectors to one file ******* \n'
rm $TESTVECTORS 2> /dev/null
cat *.patterns > $TESTVECTORS


echo $'\n ****** Coverage Analysis ******* \n'
$coverage -d $INLINE -r $RARENODES -c $CYCLES -v $TESTVECTORS -n 4 -i 10 -t $TOP


echo $'\n ****** Implant Detection ******* \n'
$midetect -g $GOLDEN -s $SUSPECTED -c $CYCLES -v $TESTVECTORS -o functionaldiff.txt

if [ -s functionaldiff.txt ] 
then
	 echo $'\n ****** MALICIOUS IMPLANTS DETECTED ******* \n'

else
	 echo $'\n ****** NO MALICIOUS IMPLANTS DETECTED ******* \n'
fi


echo $'\n ****** Cleaning Meta files ******* \n'
rm -r csrc *.daidir 2> /dev/null
rm *.ys *.key *.tcl *.run *.svf 2> /dev/null
