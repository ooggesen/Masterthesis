/*
 * @file fragment_tb.cpp
 *
 * @brief Contains fragment testbench
 *
 * TO USE THIS TESTBENCH with the csim, declare the following streams as static:
 * 	segment_meta
 * 	segment_data
 * 	segment_end
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "test_bench.hpp"
#include "../fragment.hpp"

#define NUM_TESTS 5

using namespace std;
int fragment_tb(){
	cout << "**********************************" << endl;
	cout << "     Testing fragment kernel      " << endl;
	cout << "**********************************" << endl;
	int errors = 0;

	//Generating input data
	cout << "Generating enough data for at least " << NUM_TESTS << " big chunk segments for the coarse grained fragment kernel." << endl << endl;

	hls::stream< ap_uint< 64 > > test_data("test_data");
	hls::stream< ap_uint< 8 > > compare_data("compare_data");
	hls::stream< bool > test_end("test_end");
	hls::stream< c_size_t > test_size("test_size"), compare_size("compare_size");
	generate_test_data(NUM_TESTS, test_data, compare_data, test_size, compare_size, test_end);

	//Running
	cout << "Running the fragment kernel." << endl << endl;

	hls::stream< bc_packet > out_meta("out_meta"), buffer_meta("buffer_meta");
	hls::stream< c_data_t > out_data("out_data"), buffer_data("buffer_data");
	hls::stream< bool > out_end("out_end"), buffer_end("buffer_end");
	bool end = false;
	while(!end){
		fragment(test_data, test_size, test_end, buffer_meta, buffer_data, buffer_end);

		for (int i = 0 ; i < NP_REFINE ; i ++){
			if (buffer_end.read()){
				out_end.write(true);

				end = true;
				break;
			}
			out_end.write(false);

			bc_packet meta = buffer_meta.read();
			out_meta.write(meta);

			for (int i = 0 ; i < meta.size ; i += W_DATA / 8){
				out_data.write(buffer_data.read());
			}
		}

		if(!buffer_meta.empty() || !buffer_data.empty() || !buffer_end.empty()){
			cerr << "WARNING: Kernel returned more than two small chunk." << endl;
			errors++;
		}
	}

	//Checking
	cout << "Checking the results." << endl << endl;
	l1_pos_t l1 = 0;
	c_size_t file_length = 0;
	while(!out_meta.empty()){
		cout << "Checking the " << l1 << "th big chunk.-----" << endl;

		//check end flag
		end = out_end.read();

		if (end){
			cerr << "Wrong end flag." << endl;
			errors++;
		}

		bc_packet current_bc = out_meta.read();
		//print_test_data(current_bc);

		//Check meta data
		if (current_bc.size == 0){
			cerr << "Segmented an empty big chunk." << endl;
			errors++;
		} else if (current_bc.size < BIG_CHUNK_SIZE/8) {
			cerr << "Big chunk smaller than minimum amount. Is ok if at end of file." << endl;
		}

		file_length += current_bc.size;

		if (current_bc.l1_pos != l1){
			cerr << "Wrong positional argument." << endl;
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
				cerr << "Wrong data detected, at transmission of " << (double) i/hls::ceil((double) current_bc.size.to_long()*8 / W_DATA)*100 << "% of big chunk." << endl;
				cerr << "expected: " << hex << c_compare_buffer << endl;
				cerr << "received: " << hex << c_out_buffer << endl << endl;
				errors++;
			}
		}

		l1++;
	}

	if (file_length != compare_size.read()){
		cerr << "Data lost along pipeline. Size mismatch." << endl;
		errors++;
	}

	if (!out_end.read()){
		cerr << "Wrong end flag at end of stream." << endl;
		errors++;
	}

	if (!compare_data.empty()){
		cerr << "Data lost along pipeline. Data not read in." << endl;
		errors++;
	}

	if (errors == 0){
		cerr << "All tests passed." << endl;
	} else {
		cerr << "Test failed." << endl;
	}

	return errors;
}
