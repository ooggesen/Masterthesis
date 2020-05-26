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

#define NUM_TESTS 10

using namespace std;
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

	cout << "Generating input data." << endl;
	bus_packet test_data[NUM_TESTS];
	for (int td = 0; td < NUM_TESTS; td ++){
		bus_packet *bp = test_data + td;

		for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
			bp->data[i] = rand() % (1 << 31);
		}

		bp->size = rand() % 512 + 256;
		bp->l1_pos = 0;
		bp->l2_pos = 0;
		bp->is_duplicate = false;
		bp->end = (td != SC_ARRAY_SIZE - 1);

		print_test_data(test_data[td]);
	}

	//running dedup
	cout << "Starting dedup kernel." << endl;

	bus_packet responses[NUM_TESTS];
	for (int i = 0; i < NUM_TESTS; i++){
		cout < "|";
		dedup(test_data[i], responses[i]);
	}

	cout << endl << "Dedup run finished." << endl;

	//check result TODO: write checking of results
	int errors = 0, retval;

	if (errors == 0){
		cout << "Test passed." << endl;
	}else {
		cout << "Test failed." << endl;
	}

	//Destruct file streams
	ofile.close();

	return errors;

}
