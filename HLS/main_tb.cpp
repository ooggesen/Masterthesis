/*
 * This file contains the main function for calling different tb
 *
 * Usage:
 * 	uncomment the testbenches you want to run,
 * 	but only uncomment one testbench under the category top testbenches.
 */

#include "test_bench.hpp"

int main(){
	int retval = 0;

	//INDIVIDUAL TESTBENCHES
//	retval += bram_tb();
//	std::cout << std::flush;
//	retval += copy_tb();
//	std::cout << std::flush;
//	retval += dedup_tb();
//	std::cout << std::flush;
//	retval += reorder_tb();
//	std::cout << std::flush;
//	retval += reorder_buffer_tb();
//	std::cout << std::flush;
//	retval += rabin_tb();
//	std::cout << std::flush;
//	retval += fragment_refine_tb();
//	std::cout << std::flush;
//	retval += fragment_tb();
//	std::cout << std::flush;

	//TOP TESTBENCHES
//	retval += top_refine_tb();
	retval += top_tb();


	return retval;
}
