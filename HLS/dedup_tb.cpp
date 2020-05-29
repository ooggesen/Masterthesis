/*
 * This file contains the testbench for the dedup kernel
 *
 *
 */
#include "dedup.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
using namespace std;

#define NUM_TESTS 10

struct test_result_pair{
	bus_packet test;
	bool is_duplicate;
};

void print_test_data(bus_packet test_data){
	cout << "-----" << endl;

	//cout << SC_ARRAY_SIZE << endl;
	cout << "data: " << endl;
	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
		cout << i + 1 << ": " << hex << test_data.data[i] << endl;
	}
	cout << dec <<"size: " << test_data.size <<  endl;
	cout << "l1 pos: " << test_data.l1_pos << endl;
	cout << "l2 pos: " << test_data.l2_pos << endl;
	cout << "dup: " << test_data.is_duplicate << endl;
	cout << "end: " << test_data.end << endl;

	cout << "-----" << endl;
}


/*
 * dedup testbench
 */
int main()
{
	cout << "**********************************" << endl;
	cout << "       Testing dedup kernel       " << endl;
	cout << "**********************************" << endl;

	//file to write the results
	ofstream ofile("../../../output.dat");
	if (!ofile){
		cout << "Can not open file!" <<  endl;
		return 1;
	}

	//Generating input data
	cout << "Generating " << NUM_TESTS << " tests for the dedup kernel." << endl;
	test_result_pair test_data[NUM_TESTS];
	for (int td = 0; td < NUM_TESTS; td ++){
		test_result_pair *trp = test_data + td;

		//in 20 percent of cases create duplicate for test
		if (rand() % 100 < 20){
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


		trp->test.l1_pos = 0;
		trp->test.l2_pos = 0;
		trp->test.is_duplicate = false;
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

	//Destruct file streams
	ofile.close();

	return errors;

}
