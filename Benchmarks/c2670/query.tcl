set_messages -nodisplay
set_commands noabort
read_image design.image
set_faults -model stuck
add_atpg_constraints a 1   -module c2670 N3038 

add_faults  -stuck 0 N3038

set_atpg -merge high -full_seq_time {60 240} -full_seq_atpg
run_atpg
set_messages -display
report_patterns -all -internal
#write_patterns $testPatterns -format stil -replace
exit