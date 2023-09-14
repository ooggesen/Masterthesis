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
set data_mul 1.3034 
#insert the nth root of (data_max/data_start) above


#prepare vitis project
open_project Dedup
set_top top

open_solution solution1
set_part {xczu9eg-ffvb1156-2-e}
create_clock -period 10 -name default

#Files that need include the parsec rabin algorithm
add_files "src/fragment.cpp src/fragment_refine.cpp src/fragment.hpp src/fragment_refine.hpp src/top.cpp src/top.hpp" -cflags "-I../../../..src"

source "./Dedup/solution1/directives.tcl"

#synthesize the top file
#csynth_design

#run cosims for data generation
set result_dir [file join $result_dir $system_time]
exec mkdir -v $result_dir

for {set dt_it 0} {$dt_it < [array size data_type]} {incr dt_it} {
	
	set type_dir [file join $result_dir $data_type($dt_it)] 
	
	exec mkdir -v $type_dir
	
	for {set i $data_start} {$i < $data_max} {set i [expr {$i * $data_mul}]} { #about 20 data points
	
		set size_dir [file join $type_dir $i] 
		exec mkdir -v $size_dir
		
		#cosim
		cosim_design -argv [format "-s %s -d %s" $i $data_type($dt_it)]
		#move reports to report folder
		exec mv -v $solution_dir $size_dir
	}
}

exit
