/*
 * This file contains the main function for calling different tb
 */

#include "test_bench.hpp"

int main(){
	int retval;

	//retval = bram_tb();
	//retval = copy_tb();
	//retval = dedup_tb();
	//retval = reorder_tb();
	//retval = reorder_buffer_tb();
	retval = rabin_tb();


	return retval;
}
