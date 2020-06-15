/*
 * This file contains the testbench for the coarse grained fragmentation kernel
 * -> tests the rabin segmentation as well as metadata
 */

#include "test_bench.hpp"
#include "fragment.hpp"

#define NUM_TESTS 1

using namespace std;
int fragment_tb(){
	cout << "**********************************" << endl;
	cout << "     Testing fragment kernel      " << endl;
	cout << "**********************************" << endl;

	//Generating input data
	cout << "Generating enough data for at least " << NUM_TESTS << " big chunk segments for the coarse grained fragment kernel." << endl << endl;

	hls::stream< ap_uint< 8 > > test_data, compare_data;
	generate_test_data(NUM_TESTS, test_data, compare_data);

	//Running
	cout << "Running the fragment kernel." << endl << endl;

	hls::stream< bc_packet > out_stream;
	fragment(test_data, true, out_stream);

	//Checking
	cout << "Checking the results." << endl << endl;
	int counter = 0, errors = 0;
	while(!out_stream.empty()){
		cout << "Checking the " << counter << "th big chunk.-----" << endl;

		bc_packet current_bc = out_stream.read();
		//print_test_data(current_bc);

		if (current_bc.size == 0){
			cout << "Segmented an empty big chunk." << endl;
			errors++;
		}


		for (int  i = 0 ; i < current_bc.size ; i++){
			if (current_bc.data[(int) (8 * i) / W_DATA_BIG_CHUNK].range(7 + (8*i % W_DATA_BIG_CHUNK), 8*i % W_DATA_BIG_CHUNK) != compare_data.read()){
				cout << "Wrong data detected." << endl;
				errors++;
			}
		}

		if (out_stream.empty() && !current_bc.end){
			cout << "End flag not correctly set." << endl;
			errors++;
		}
		counter++;
	}

	if (errors == 0){
		cout << "All tests passed." << endl;
	} else {
		cout << "Test failed." << endl;
	}

	return errors;
}
