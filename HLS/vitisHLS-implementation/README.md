# VitisHLS dedup kernel

This folder contains the dedup version designed with VitisHLS for the master thesis of Ole Oggesen.

We used the version 2021.2 of VitisHLS.

## structure


The file "src/top.cpp" contains the top module and each pipeline stage is in a file with the same name are in the same folder. 
The testbenches for the individual kernels, as well for the top module have the same name as their device under test, with a subsequent suffix _tb. 
The main_tb.cpp file contains the main function for vitisHLS with all testbench calls.
All testbenches are stored in the folder "src/Testbenches/".

## run instructions

### GUI version
Create a new project in VitisHLS. 
Add all testbenches, as well as the files test_bench.cpp and test_bench.hpp to the testbenches. 
Add all other *.hpp and *.cpp files to source files. 
Select the top module for synthesis and follow the instructions in main_tb.cpp for testing.

### skripted version
The tcl skript "./testDedup.tcl" contains the generation of a project, synthesis and subsequent cosim runs to generate the test data which is used in the master thesis.
It can be used to create and test the vitis_hls project. Outcomment steps, like sythesis, cosim or csim in the file as needed.

## todos

Change the bram hash table to have a prime size.
