/*
 * This file contains the main function for calling different tb
 *
 * Usage:
 * 	uncomment the testbenches you want to run,
 */

#include "test_bench.hpp"

int main(){
	int retval = 0;


//HELPER FUNCTION TESTBENCHES
//	retval += shuffle_tb();
//	std::cout << std::endl;
//	retval += generate_test_data_tb();
//	std::cout << std::endl;
//	retval += bram_tb();
//	std::cout << std::endl;
//	retval += copy_tb();
//	std::cout << std::endl;
//	retval += reorder_buffer_tb();
//	std::cout << std::endl;


//KERNEL TESTBENCHES
//	retval += fragment_tb();
//	std::cout << std::endl;
//	retval += fragment_refine_tb();
//	std::cout << std::endl;
//	retval += dedup_tb();
//	std::cout << std::endl;
//	retval += reorder_tb();
//	std::cout << std::endl;


//TOP TESTBENCH
	retval += top_tb();
	std::cout << std::endl;


	return retval;
}
