############################################################
## This file is generated automatically by Vitis HLS.
## Please DO NOT edit it.
## Copyright 1986-2021 Xilinx, Inc. All Rights Reserved.
############################################################
open_project HLS
set_top dedup
add_files HLS/bram.cpp
add_files HLS/bram.h
add_files HLS/bus_def.cpp
add_files HLS/bus_def.h
add_files HLS/dedup.cpp
add_files HLS/dedup.h
add_files HLS/main.cpp
add_files HLS/main.h
add_files Submodules/Vitis_libraries/security/L1/include/xf_security/sha1.hpp
add_files -tb HLS/bram_tb.cpp
add_files -tb HLS/dedup_tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1" -flow_target vivado
set_part {xcvu11p-flga2577-1-e}
create_clock -period 10 -name default
#source "./HLS/solution1/directives.tcl"
csim_design
csynth_design
cosim_design
export_design -format ip_catalog
