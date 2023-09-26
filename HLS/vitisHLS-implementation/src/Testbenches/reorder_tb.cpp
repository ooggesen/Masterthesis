/*
 * @file reorder_tb.cpp
 *
 * @brief Contains reorder testbench
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "../reorder.hpp"
#include "test_bench.hpp"

using namespace std;

#define NUM_TESTS BUFFER_SIZE_2*BUFFER_SIZE_1

struct test_result_pair{
	sc_packet test;
	addr_t hash;
};



/*
 * reorder tb function
 */
int reorder_tb(){
	cout << "**********************************" << endl;
	cout << "      Testing reorder kernel      " << endl;
	cout << "**********************************" << endl;
	int errors = 0;

	//Generating input data
	cout << "Generating " << NUM_TESTS << " tests for the reorder kernel." << endl;


	hls::stream< sc_packet > test_meta("test_meta"), compare_meta("compare_meta");
	hls::stream< c_data_t > test_data("test_data"), compare_data("compare_data");
	hls::stream< bool > test_end("test_end");
	generate_test_data(NUM_TESTS, true,  test_meta, test_data, test_end, compare_meta, compare_data);

	//shuffle the order of the packages to the reorder stage
	cout << "Shuffle test data." << endl;
	hls::stream< sc_packet > shuffled_meta("shuffeled_meta");
	hls::stream< c_data_t > shuffled_data("shuffeled_data");
	hls::stream< bool > shuffled_end("shuffled_end");
	shuffle(test_meta, test_data, test_end, shuffled_meta, shuffled_data, shuffled_end);

	cout << "Finished generating test data." << endl << endl;
	cout << "Running the reorder kernel." << endl;

	hls::stream< ap_uint< 64 > > out_data("out_data"), buffer_data("buffer_data");
	hls::stream < c_size_t > out_size("out_size"), buffer_size("buffer_size");
	hls::stream< bool > out_end("out_end"), buffer_end("buffer_end");

	bool end = false;
	while(!end){
		reorder(shuffled_meta, shuffled_data, shuffled_end, buffer_size, buffer_data, buffer_end);

		if (!buffer_end.empty()){
			end = buffer_end.read();
			out_end.write(end);
		}

		//size information arrives at the end of parsing
		if (!buffer_size.empty()){
			c_size_t size = buffer_size.read();
			out_size.write(size);
		}

		while(!buffer_data.empty()){
			out_data.write(buffer_data.read());
		}

		if(!buffer_size.empty() || !buffer_data.empty() || !buffer_end.empty()){
			cerr << "WARNING: Kernel returned more than two small chunk." << endl;
			errors++;
		}
	}

	cout << "Checking results." << endl << endl;
	c_size_t file_length;
	end = out_end.read();
	while(!end){
		file_length = 0;

		//check header
		if (out_data.read() != CHECKBIT){
			cerr << "Wrong checkbit." << endl;
			errors++;
		}
		file_length += 8;

		if (out_data.read() != COMPRESS_NONE){
			cerr << "Wrong compress type." << endl;
			errors++;
		}
		file_length += 8;


		//Check the data stream
		for (int td = 0 ; td < NUM_TESTS ; td++){
			cout << endl <<"Check test data nr. " << dec << td+1 << endl << "----------" << endl;
			//read package meta data
			sc_packet to_compare_meta = compare_meta.read();

			ap_uint< 64 > type = out_data.read();

			c_size_t size = out_data.read();
			file_length += 16;


			//if duplicate expect sha1 sum
			if (to_compare_meta.is_duplicate){
				cout << "Found duplicate chunk." << endl;

				file_length += 24;

				//check seperator
				//check type
				if (type != TYPE_FINGERPRINT){
					cerr << "Wrong type written to file." << endl;
					cerr << "expected: " << TYPE_FINGERPRINT << endl;
					cerr << "received: " << type << endl;
					errors++;
				}

				//check size
				if(size != 20){
					cerr << "Wrong size written to file." << endl;
					cerr << "expected: " << to_compare_meta.size << endl;
					cerr << "received: " << size << endl;
					errors++;
				}

				//check hash
				addr_t hash;
				hash.range(63, 0) = out_data.read();
				hash.range(127, 64) = out_data.read();
				ap_uint< 64 > buffer = out_data.read();
				hash.range(159, 128) = buffer.range(31, 0);

				if (hash != td){
					cerr << left << "Wrong hash" << endl;
					cerr << left << "expected: " << right << hex << setw(W_ADDR/16) << td << endl;
					cerr << left << "received: " << right << hex << setw(W_ADDR/16) << hash << endl;

					errors++;
				}

				//clean up data from compare stream
				for (int i = 0 ; i < hls::ceil((double) to_compare_meta.size.to_long()*8 / W_DATA) ; i++){
					compare_data.read();
				}
			} else {
			//if unique chunk expect chunk as output
				cout << "Received unique chunk." << endl;

				file_length += size;
				if (size % 8 != 0){
					file_length += 8 - (size % 8);//zero stuffing
				}

				//check seperator
				//check type
				if (type != TYPE_COMPRESS){
					cerr << "Wrong type written to file." << endl;
					cerr << "expected: " << TYPE_COMPRESS << endl;
					cerr << "received: " << type << endl;
					errors++;
				}

				//check size
				if(size != to_compare_meta.size){
					cerr << "Wrong size written to file." << endl;
					cerr << "expected: " << to_compare_meta.size << endl;
					cerr << "received: " << size << endl;
					errors++;
				}

				//check chunk data
				for (int i = 0 ; i < hls::ceil((double) size.to_long()*8 / W_DATA) ; i++){
					c_data_t compare = compare_data.read();
					c_data_t output;
					//build output data
					for (int j = 0 ; j < W_DATA/64 ; j++){
						if (size.to_long() > i*W_DATA/8 + j*8){
							output.range(63 + 64*j, 64*j) = out_data.read();
						} else {
							output.range(63 + 64*j, 64*j) = 0;
						}
					}

					//check
					if(output != compare){
						cerr << left << "Wrong chunk data" << endl;
						cerr << left << "expected: " << right << hex << compare << endl;
						cerr << left << "received: " << right << hex << output << endl;

						errors++;
					}
				}
			}
		}

		c_size_t read_size = out_size.read();
		if (read_size != file_length){
			cerr << "Data lost. Output_size does not match the read bytes." << endl;
			errors++;
		}

		end = out_end.read();
	}

	cout << "Finished processing." << endl << endl;

	//Check if output stream is complete
	if (!out_data.empty()){
		cerr << "Left over data in output FIFO: " << endl;
		int counter = 0;
		while (!out_data.empty()){
			cerr << hex << out_data.read() << endl;
			counter++;
		}
		errors++;
		cerr << endl;
		cerr << "Number of bytes which were left in FIFO: " << counter << endl;
	}

	if (!compare_data.empty()){
		cerr << "Not processed compare data:" << endl;
		int counter = 0;
		while (!compare_data.empty()){
			cerr << hex << compare_data.read() << endl;
			counter++;
		}
		errors++;
		cerr << "Number of c_data_t chunks left in compare stream: " << counter << endl;
	}


	//Final report
	if (errors == 0){
		cout << " All " << dec << NUM_TESTS << " tests passed." << endl;
	} else {
		cout << "Tests failed." << endl;
	}

	return errors;
}
