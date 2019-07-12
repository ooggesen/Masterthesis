/*
 * This file contains all trestbenches for the testbenches helper functions
 */
#include "test_bench.hpp"
#include "bus_def.hpp"
#include "hls_math.h"
using namespace std;

#define NUM_TESTS 10

int generate_test_data_tb(){
	cout << "****************************************" << endl;
	cout << "  Testing generate test data function   " << endl;
	cout << "****************************************" << endl;

	//Generating input data
	cout << "Generating " << NUM_TESTS << " tests for the generate test data function." << endl;

	hls::stream< sc_packet > test_meta("test_meta"), compare_meta("compare_meta");
	hls::stream< c_data_t > test_data("test_data"), compare_data("compare_data");
	generate_test_data(NUM_TESTS, true, test_meta, test_data, compare_meta, compare_data);

	cout << endl << endl << "Checking data." << endl;
	int errors = 0;
	for (int i = 0 ; i < NUM_TESTS ; i++){
		sc_packet t_meta = test_meta.read();
		sc_packet c_meta = compare_meta.read();

		if (t_meta != c_meta){
			cout << "Meta data is not matching." << endl;
			errors++;
		}

		for (int j = 0 ; j < hls::ceil((double) t_meta.size.to_long()*8/W_DATA) ; j++){
			if (test_data.read() != compare_data.read()){
				cout << "Data does not match." << endl;
				errors++;
			}
		}
	}

	if (!compare_data.empty() || !test_data.empty()){
		cout << "Generated too much data." << endl;
		errors++;
	}
	return errors;
}



int shuffle_tb(){
	cout << "*****************************" << endl;
	cout << "  Testing shuffle function   " << endl;
	cout << "*****************************" << endl;

	//Generating input data
	cout << "Generating " << NUM_TESTS << " tests for the shuffle function." << endl;
	cout << "This test assumes that so few data is generated, that all small chunks belong to the same big chunk." << endl;

	hls::stream< sc_packet > test_meta("test_meta"), compare_meta("compare_meta");
	hls::stream< c_data_t > test_data("test_data"), compare_data("compare_data");
	generate_test_data(NUM_TESTS, true, test_meta, test_data, compare_meta, compare_data);


	cout << "Run the shuffle function." << endl << endl;;
	hls::stream< sc_packet > shuffled_meta("shuffled_meta");
	hls::stream< c_data_t > shuffled_data("shuffled_data");
	shuffle(test_meta, test_data, shuffled_meta, shuffled_data);

	//Build correct data order
	cout << "Finished." << endl << endl;
	hls::stream< c_data_t > sorted_data[NUM_TESTS];
	for (int i = 0 ; i < NUM_TESTS ; i++){
		sc_packet tmp_meta = shuffled_meta.read();

		for (int j = 0 ; j < hls::ceil((double) tmp_meta.size.to_long()*8 / W_DATA) ; j++){
			//assumes that all tests belong to one big chunk of data
			sorted_data[tmp_meta.l2_pos].write(shuffled_data.read());
		}
	}

	//Checking
	int errors = 0;
	cout << "Checking data coherency." << endl;
	for (int i = 0 ; i < NUM_TESTS ; i++){
		sc_packet tmp_compare = compare_meta.read();
		for(int j = 0 ; j < hls::ceil((double) tmp_compare.size.to_long()*8 / W_DATA) ; j++){
			if (sorted_data[i].read() != compare_data.read()){
				cout << "Data missing." << endl;
				errors++;
			}
		}
	}

	if (!compare_data.empty()){
		cout << "Data was lost in function. Compare stream is not empty." << endl;
		while (!compare_data.empty()){
			cout << compare_data.read() << endl;
		}
		cout << endl;
	}

	return errors;
}