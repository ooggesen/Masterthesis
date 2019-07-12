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

	hls::stream< ap_uint< 64 > > test_data("test_data");
	hls::stream< ap_uint< 8 > > compare_data("compare_data");
	hls::stream< bool > test_end("test_end");
	hls::stream< c_size_t > test_size("test_size"), compare_size("compare_size");
	generate_test_data(NUM_TESTS, test_data, compare_data, test_size, compare_size, test_end);

	//Running
	cout << "Running the fragment kernel." << endl << endl;

	hls::stream< bc_packet > out_meta("out_meta");
	hls::stream< c_data_t > out_data("out_data");
	hls::stream< bool > out_end("out_end");
	fragment(test_data, test_size, test_end, out_meta, out_data, out_end);

	//Checking
	cout << "Checking the results." << endl << endl;
	int errors = 0;
	l1_pos_t l1 = 0;
	bool end;
	c_size_t file_length = 0;
	while(!out_meta.empty()){
		cout << "Checking the " << l1 << "th big chunk.-----" << endl;

		//check end flag
		end = out_end.read();

		if (end){
			cout << "Wrong end flag." << endl;
			errors++;
		}

		bc_packet current_bc = out_meta.read();
		//print_test_data(current_bc);

		//Check meta data
		if (current_bc.size == 0){
			cout << "Segmented an empty big chunk." << endl;
			errors++;
		}

		file_length += current_bc.size;

		if (current_bc.l1_pos != l1){
			cout << "Wrong positional argument." << endl;
			errors++;
		}

		//Check data
		for (int  i = 0 ; i < hls::ceil((double) current_bc.size.to_long()*8 / W_DATA) ; i++){
			c_data_t c_out_buffer = out_data.read();
			c_data_t c_compare_buffer;
			//build compare data as c_data_t
			for (int j = 0 ; j < W_DATA/8 ; j++){
				int pos = j*8;
				if (current_bc.size.to_long() > i*W_DATA/8 + j){
					c_compare_buffer.range(7 + pos, pos) = compare_data.read();
				} else {
					c_compare_buffer.range(7 + pos, pos) = 0;
				}
			}
			if (c_out_buffer != c_compare_buffer){
				cout << "Wrong data detected, at transmission of " << (double) i/hls::ceil((double) current_bc.size.to_long()*8 / W_DATA)*100 << "% of big chunk." << endl;
				cout << "expected: " << hex << c_compare_buffer << endl;
				cout << "received: " << hex << c_out_buffer << endl << endl;
				errors++;
			}
		}

		l1++;
	}

	if (file_length != compare_size.read()){
		cout << "Data lost along pipeline. Size mismatch." << endl;
		errors++;
	}

	if (!out_end.read()){
		cout << "Wrong end flag at end of stream." << endl;
		errors++;
	}

	if (!compare_data.empty()){
		cout << "Data lost along pipeline. Data not read in." << endl;
		errors++;
	}

	if (errors == 0){
		cout << "All tests passed." << endl;
	} else {
		cout << "Test failed." << endl;
	}

	return errors;
}
