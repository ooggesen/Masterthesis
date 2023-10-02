#synthesize your project and run this script to generate delay data

#path/to/solution
set solution_dir [file join [pwd] "Dedup/solution1/sim/report/"]

#path/to/report/folder
set result_dir [file join [pwd] "reports"]

#get time and date
set system_time [clock seconds]
set system_time [clock format $system_time -format %d%m%y%H%M%S]

#set different data type keywords to test on
set data_type(0) duplicate
set data_type(1) semi
set data_type(2) unique

#set data size specifiers
set data_start 5000
set data_max 1000000
set data_mul 1.32162 
#insert the (n-1)th root of (data_max/data_start) above, where n is the ammound of data points per data type


#prepare vitis project
open_project Dedup
set_top top

open_solution solution1
set_part {xczu9eg-ffvb1156-2-e}
create_clock -period 10 -name default

#Files that need include the parsec rabin algorithm
add_files "src/fragment.cpp src/fragment_refine.cpp src/fragment.hpp src/fragment_refine.hpp src/top.cpp src/top.hpp" -cflags "-I../../../../src"

#source files
add_files src/bram.cpp
add_files src/bram.hpp
add_files src/bus_def.cpp
add_files src/bus_def.hpp
add_files src/dedup.cpp
add_files src/dedup.hpp
add_files src/parsec.hpp
add_files src/parsec/rabin.cpp
add_files src/parsec/rabin.hpp
add_files src/reorder.cpp
add_files src/reorder.hpp
add_files src/reorder_buffer.cpp
add_files src/reorder_buffer.hpp
add_files src/scheduler.cpp
add_files src/scheduler.hpp
add_files src/sha1.hpp

#testbenches
add_files -tb src/Testbenches/bram_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/bus_def_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/dedup_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/fragment_refine_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/fragment_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/main_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/reorder_buffer_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/reorder_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/scheduler_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/test_bench.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/test_bench.hpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/test_bench_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/Testbenches/top_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"

config_rtl -reset state
source "./Dedup/solution1/directives.tcl"

#simulate the top file
#csim_design -clean

#synthesize the top file
csynth_design

#run cosims for data generation
set result_dir [file join $result_dir $system_time]
exec mkdir -v $result_dir

for {set dt_it 0} {$dt_it < [array size data_type]} {incr dt_it} {
	
	set type_dir [file join $result_dir $data_type($dt_it)] 
	
	exec mkdir -v $type_dir
	
	for {set i $data_start} {$i < $data_max} {set i [expr {$i * $data_mul}]} { 
	
		set i [expr int($i)]
		set size_dir [file join $type_dir $i] 
		exec mkdir -v $size_dir
		
		#cosim
		cosim_design -argv [format "-s %s -d %s" $i $data_type($dt_it)]
		#move reports to report folder
		exec mv -v $solution_dir $size_dir
	}
}

exit
