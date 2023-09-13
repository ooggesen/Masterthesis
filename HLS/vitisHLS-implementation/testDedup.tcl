#synthesize your project and run this script to generate delay data

#path/to/solution
set work_dir "/mnt/ole/Documents/coderepo/HLS/vitisHLS-implementation/Dedup/solution1" 
#path to folder for storing cosim reports
set result_dir "/mnt/ole/Documents/coderepo/HLS/vitisHLS-implementation/reports"	

#get time and date
set system_time [clock seconds]
set system_time [clock format $system_time -format %D||%H:%M:%S]

#set different data type keywords to test on
set data_type(0) duplicate
set data_type(1) semi
set data_type(2) unique

open_project Dedup
set_top top

open_solution solution1
set_part {xczu9eg-ffvb1156-2-e}
create_clock -period 10 -name default

#Files that need include the parsec rabin algorithm
add_files "src/fragment.cpp src/fragment_refine.cpp src/fragment.hpp src/fragment_refine.hpp src/top.cpp src/top.hpp" -cflags "-I../../../..src"

source "./Dedup/solution1/directives.tcl"

#synthesize
csynth_design

#run cosims for data generation
exec mkdir -v $result_dir/$system_time

for {set dt_it 0} {$dt_it < array size data_type} {incr dt_it}

	exec mkdir -v $result_dir/$system_time/$data_type($dt_it)
	
	for {set i 1000} {$i < 1000000} {set i [expr {$i + 2000}]} {
	
		exec mkdir -v $result_dir/$system_time/$data_type($dt_it)/$i
		#cosim
		cosim_design -argv -s $i -d $data_type($dt_it)
		#move reports to report folder
		exec mv -v $work_dir/sim/report/ $result_dir/$system_time/$data_type($dt_it)/$i
	}
}

exit
