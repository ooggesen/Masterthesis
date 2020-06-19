/*
 * This file contains all testbenches for the top module. This refers to testbenches that check intermediate results as well as the whole programm.
 * -> top functions which output intermediate results, must be created by adapting the top function to output intermediate values
 */


#include "test_bench.hpp"
#include "top.hpp"

#define NUM_TESTS 4
using namespace std;


/*
 * This testbench tests a top module without the dedup kernel
 * -> adapt the top function accordingly
 * -> remove print header and print seperator functions from the reorder kernel
 */
int top_refine_tb(){
	cout << "*********************************************************" << endl;
	cout << "   Testing whole module until fragment refine kernel     " << endl;
	cout << "*********************************************************" << endl;

	//Generating input data
	cout << "Generating enough data for at least " << NUM_TESTS << " big chunk segments for the top function." << endl << endl;

	hls::stream< ap_uint< 8 > > test_data, compare_data;
	generate_test_data(NUM_TESTS, test_data, compare_data);

	//Running
	cout << "Running the dut." << endl << endl;

	hls::stream< ap_uint< 8 > > out_stream;
	top(test_data, true, out_stream);

	//Checking results
	int counter = 0, errors = 0;
	while(!out_stream.empty()){
		//cout << "Checking the " << counter << "th small chunk.-----" << endl;

		ap_uint< 8 > compare_byte = compare_data.read();
		ap_uint< 8 > out_byte = out_stream.read();

		if (out_byte != compare_byte){
			cout << "Wrong data output." << endl;
			cout << "Byte pos: " << counter << " Expected: " << hex << compare_byte << ",but received " << out_byte << endl;
			errors++;
		}
		counter++;
	}

	if (!compare_data.empty()){
		cout << "Lost data along pipeline." << endl;
		errors++;
	}

	if (errors == 0){
		cout << "All tests passed." << endl;
	} else {
		cout << "Test failed." << endl;
	}


	return errors;
}
