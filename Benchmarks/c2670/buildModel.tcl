
set_messages -nodisplay
set_commands noabort
set readFile "synthesised.v"
set topModule "c2670"
#set writeFile "design_output.v"
set testPatterns "test_patterns.stil"
set libFile [getenv techmap]/saed90nm.v
read_netlist $readFile
read_netlist $libFile -library
#report_modules -summary
#report_modules -error
#report_modules -undefined
run_build_model $topModule
run_drc
set_faults -model stuck
write_image design.image -replace
exit
    