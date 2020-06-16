/*
 * This file contains all testbenches for the top module. This refers to testbenches that check intermediate results as well as the whole programm.
 * -> top functions which output intermediate results, must be created by adapting the top function to output intermediate values
 */


#include "test_bench.hpp"
#include "top.hpp"

#define NUM_TESTS 2
using namespace std;


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

	hls::stream< sc_packet > out_stream;
	top(test_data, true, out_stream);

	//Checking results
	int counter = 0, errors = 0;
	while(!out_stream.empty()){
		cout << "Checking the " << counter << "th small chunk.-----" << endl;

		sc_packet current_sc = out_stream.read();
		//print_test_data(current_bc);

		if (current_sc.size == 0){
			cout << "Segmented an empty small chunk." << endl;
			errors++;
		}

		//can not check output since data is shuffeled by parallel fragment refine kernels
		for (int  i = 0 ; i < current_sc.size.to_long() ; i++){
			ap_uint< 8 > compare_byte  = compare_data.read();
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
