
set_messages -nodisplay
set_commands noabort
read_image design.image
#add_faults -all
drc

add_pi_constraints 0 N3
    
test
set_faults -model stuck

add_atpg_constraints a 0   -module c2670 N1448
    

add_faults  -stuck  1 N1448
    
#list down the faults 
# for scan chain designs
set_atpg -merge high -full_seq_time {60 240} -full_seq_atpg
run_atpg
set_messages -display
report_patterns -all -internal
#write_patterns $testPatterns -format stil -replace
exit
    