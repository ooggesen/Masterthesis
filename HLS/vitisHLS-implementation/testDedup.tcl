#synthesize your project and run this script to generate delay data

#path/to/solution
set work_dir "/mnt/ole/Documents/coderepo/HLS/vitisHLS-implementation/Dedup/solution1" 
#path to folder for storing cosim reports
set result_dir "/mnt/ole/Documents/coderepo/HLS/vitisHLS-implementation/reports"	

open_project Dedup
set_top top

open_solution solution1
set_part {xczu9eg-ffvb1156-2-e}
create_clock -period 10 -name default

#Files that need include the parsec rabin algorithm
add_files "src/fragment.cpp src/fragment_refine.cpp src/fragment.hpp src/fragment_refine.hpp src/top.cpp src/top.hpp" -cflags "-I../../../..src"

source "./Dedup/solution1/directives.tcl"

for {set i 1000} {$i < 10000} {set i [expr {$i + 1000}]} {
	exec mkdir -v $result_dir/$i
    cosim_design -argv -s$i -d"unique"
    exec mv -v $work_dir/sim/report/ $result_dir/$i
}

exit
