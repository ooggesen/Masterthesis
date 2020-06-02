/*
 * This file contains the main function for calling different tb
 */

#include "test_bench.h"

int main(){
	int retval;

	//retval = bram_tb();
	//retval = copy_tb();
	//retval = dedup_tb();
	//retval = reorder_tb();
	retval = buffer_tb();


	return retval;
}
