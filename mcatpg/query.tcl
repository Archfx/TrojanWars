
#set_messages -nodisplay
#set_commands noabort
cd /home/UFAD/arunajayasena/Designs/MaximalClique/Benchmarks
read_image aes/aes.image
#add_faults -all

add_atpg_constraints a 0 -module aes _a10_rcon_1_

    

add_faults  -stuck  1 _a10_rcon_1_

    
#list down the faults 
# for scan chain designs
set_atpg -merge high -full_seq_time {60 240} -full_seq_atpg
run_atpg
set_messages -display
report_patterns -summary -internal
#write_patterns $testPatterns -format stil -replace
#exit
    