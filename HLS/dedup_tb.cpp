/*
 * This file contains the testbench for the dedup kernel
 *
 *
 */
#include "dedup.hpp"
#include "test_bench.hpp"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <time.h>
using namespace std;

#define NUM_TESTS 10

struct test_result_pair{
	sc_packet test;
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

	hls::stream< sc_packet > test_data, compare_data;
	generate_test_data(NUM_TESTS, false,  test_data, compare_data);

	//running dedup
	cout << "Starting dedup kernel." << endl;

	sc_packet responses[NUM_TESTS];
	for (int i = 0; i < NUM_TESTS; i++){
		cout << "|";
		sc_packet bp  = test_data.read();
		dedup(bp, responses[i]);
	}

	cout << endl << "Dedup run finished." << endl;

	//check results
	int errors = 0;

	cout << "Checking results." << endl;

	for (int i = 0 ; i < NUM_TESTS ; i++){
		sc_packet current  = compare_data.read();
		if (current.is_duplicate != responses[i].is_duplicate){
			cout << left << "Wrong prediction:" << endl;
			cout << left << "test data: " << right << setw(2) << current.is_duplicate << endl;
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
