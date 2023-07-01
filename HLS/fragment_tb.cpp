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

	hls::stream< ap_uint< 8 > > test_data("test_data"), compare_data("compare_data");
	generate_test_data(NUM_TESTS, test_data, compare_data);

	//Running
	cout << "Running the fragment kernel." << endl << endl;

	hls::stream< bc_packet > out_meta("out_meta");
	hls::stream< c_data_t > out_data("out_data");
	fragment(test_data, true, out_meta, out_data);

	//Checking
	cout << "Checking the results." << endl << endl;
	int errors = 0;
	int l1 = 0;
	while(!out_meta.empty()){
		cout << "Checking the " << l1 << "th big chunk.-----" << endl;

		bc_packet current_bc = out_meta.read();
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
		for (int  i = 0 ; i < hls::ceil((double) current_bc.size.to_long()*8 / W_DATA) ; i++){
			c_data_t c_out_buffer = out_data.read();
			c_data_t c_compare_buffer;
			for (int j = 0 ; j < W_DATA/8 ; j++){
				if (current_bc.size.to_long() > i*W_DATA/8 + j){
					c_compare_buffer.range(7 + j*8, j*8) = compare_data.read();
				} else {
					c_compare_buffer.range(7 + j*8, j*8) = 0;
				}
			}
			if (c_out_buffer != c_compare_buffer){
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
