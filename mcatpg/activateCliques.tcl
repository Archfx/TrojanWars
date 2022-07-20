
set_messages -nodisplay
set_commands noabort
cd /home/UFAD/arunajayasena/Designs/MaximalClique/Benchmarks
read_image c2670/c2670.image
#add_faults -all

add_atpg_constraints MC79 0 -module c2670 N1656

add_faults  -stuck  1 N1656
#list down the faults 
# for scan chain designs
set_atpg -merge high -verbose -full_seq_time {600 3600} -full_seq_atpg
run_atpg
set_messages -display
report_patterns -all -internal
#write_patterns $testPatterns -format stil -replace
exit
    