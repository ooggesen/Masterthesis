/*
 * This file contains the testbench for the coarse grained fragmentation kernel
 * -> tests the rabin segmentation as well as metadata
 */

#include "test_bench.hpp"
#include "fragment.hpp"

#define NUM_TESTS 3

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
	int errors = 0;
	int l1 = 0;
	while(!out_stream.empty()){
		cout << "Checking the " << l1 << "th big chunk.-----" << endl;

		bc_packet current_bc = out_stream.read();
		//print_test_data(current_bc);

		//Check meta data
		if (current_bc.size == 0){
			cout << "Segmented an empty big chunk." << endl;
			errors++;
		}

		if (current_bc.l1_pos.to_int() != l1){
			cout << "Wrong positional argument." << endl;
			errors++;
		}

		//Check data
		for (int  i = 0 ; i < current_bc.size ; i++){
			if (current_bc.data[(int) (8 * i) / W_DATA_BIG_CHUNK].range(7 + (8*i % W_DATA_BIG_CHUNK), 8*i % W_DATA_BIG_CHUNK) != compare_data.read()){
				cout << "Wrong data detected." << endl;
				errors++;
			}
		}

		l1++;
	}

	if (!compare_data.empty()){
		cout << "Data lost along pipeline." << endl;
		errors++;
	}

	if (errors == 0){
		cout << "All tests passed." << endl;
	} else {
		cout << "Test failed." << endl;
	}

	return errors;
}
