
set_messages -nodisplay
set_commands noabort
cd /home/UFAD/arunajayasena/Designs/MaximalClique/Benchmarks
set readFile "c3540/c3540.v"
set topModule "c3540"
set writeFile "c3540/c3540_output.v"
set testPatterns "c3540/c3540_patterns.stil"
set libFile "Libraries/saed90nm.v"
read_netlist $readFile
read_netlist $libFile -library
#report_modules -summary
#report_modules -error
#report_modules -undefined
run_build_model $topModule
run_drc
set_faults -model stuck
write_image c3540/c3540.image -replace
exit
    