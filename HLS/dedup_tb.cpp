/*
 * This file contains the testbench for the dedup kernel
 *
 *
 */
#include "dedup.h"
#include "test_bench.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <time.h>
using namespace std;

#define NUM_TESTS 100

struct test_result_pair{
	bus_packet test;
	bool is_duplicate;
};


/*
 * dedup testbench
 */
int dedup_tb()
{
	cout << "**********************************" << endl;
	cout << "       Testing dedup kernel       " << endl;
	cout << "**********************************" << endl;

	//Generating input data
	cout << "Generating " << NUM_TESTS << " tests for the dedup kernel." << endl;

	srand(time(NULL)); //set rand seed TODO: make this set by main argument as well for reproducability

	test_result_pair test_data[NUM_TESTS];
	for (int td = 0; td < NUM_TESTS; td ++){
		test_result_pair *trp = test_data + td;

		//in 20 percent of cases create duplicate for test
		if (rand() % 100 < 20 && td != 0){
			for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
				trp->test.data[i] = (trp - 1)->test.data[i];
			}
			trp->test.size = (trp - 1)->test.size;

			trp->is_duplicate = true;
		} else {
			for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
				trp->test.data[i] = rand() % (1 << 31);
			}
			trp->test.size = rand() % 512 + 256;

			trp->is_duplicate = false;
		}

		trp->test.hash = 0;
		trp->test.l1_pos = 0;//no effect in the dedup kernel
		trp->test.l2_pos = 0;//no effect in the dedup kernel
		trp->test.is_duplicate = false;
		trp->test.last_l2_chunk = false; //no effect in the dedup kernel
		trp->test.end = (td == SC_ARRAY_SIZE - 1);

		print_test_data(trp->test);
	}

	//running dedup
	cout << "Starting dedup kernel." << endl;

	bus_packet responses[NUM_TESTS];
	for (int i = 0; i < NUM_TESTS; i++){
		cout << "|";
		dedup(test_data[i].test, responses[i]);
	}

	cout << endl << "Dedup run finished." << endl;

	//check results
	int errors = 0;

	cout << "Checking results." << endl;

	for (int i = 0 ; i < NUM_TESTS ; i++){
		if (test_data[i].is_duplicate != responses[i].is_duplicate){
			cout << left << "Wrong prediction:" << endl;
			cout << left << "test data: " << right << setw(2) << test_data[i].is_duplicate << endl;
			cout << left << "responses: " << right << setw(2) << responses[i].is_duplicate << endl;
			++errors;
		}
	}

	if (errors == 0){
		cout << "Tests passed." << endl;
	}else {
		cout << errors << " out of " << NUM_TESTS <<" tests failed." << endl;
	}

	return errors;

}
