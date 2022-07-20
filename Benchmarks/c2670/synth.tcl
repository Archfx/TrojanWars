

set_messages -nodisplay

set_commands noabort

set readFile  preSynth.v

set topModule c2670

set writeFile "synthesised.v"

set_app_var search_path "."

set_app_var target_library [getenv techmap]/saed90nm_typ.db

set_app_var link_library [getenv techmap]/saed90nm_typ.db


read_file -format verilog $readFile

current_design $topModule

link

create_clock clk -period 10

compile -map_effort medium

set_flatten

write -hierarchy -format verilog -output $writeFile

exit

    