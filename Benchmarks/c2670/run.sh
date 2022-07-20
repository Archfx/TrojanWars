DESIGN = "c2670"
TOP = "c2670"
CLK = "clk"
RST = "rst"
INLINE = "inline.v"
CYCLES = "3"
N-CRITERION = "100"
RARE_NODE_FILE = "rare_nodes.txt"

($flatten) -d $(DESIGN) -t $(TOP) -k $(CLK) -s $(RST) -c $(CYCLES)
($midrare) -d $(INLINE) -i 10000 -c $(CYCLES)
($midsynth) -t $(TOP) -d $(INLINE) -r $(RARE_NODE_FILE)
($ndetect) -d $(INLINE) -i 1000 -c $(CYCLES) -t $(TOP) -n $(N-CRITERION)
($ndatpg) -t $(TOP) -d synthesised.v -r $(RARE_NODE_FILE) -c $(CYCLES) -n $(N-CRITERION) -k $(CLK) -s $(RST)
($coverage) -d $(INLINE) -r $(RARE_NODE_FILE) -c $(CYCLES) -v c2670_ndatpg.patterns -n 2 -i 10 -t $(TOP)
