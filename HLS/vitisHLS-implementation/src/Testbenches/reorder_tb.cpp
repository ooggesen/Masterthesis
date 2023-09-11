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

	hls::stream< ap_uint< 64 > > output_data("output_data");
	hls::stream < c_size_t > output_size("output_size");
	hls::stream< bool > output_end("output_end");
	reorder(shuffled_meta, shuffled_data, shuffled_end, output_size, output_data, output_end);

	cout << "Checking results." << endl << endl;
	int errors = 0;
	c_size_t file_length;
	bool end = output_end.read();
	while(!end){
		file_length = 0;

		//check header
		if (output_data.read() != CHECKBIT){
			cout << "Wrong checkbit." << endl;
			errors++;
		}
		file_length += 8;

		if (output_data.read() != COMPRESS_NONE){
			cout << "Wrong compress type." << endl;
			errors++;
		}
		file_length += 8;


		//Check the data stream
		for (int td = 0 ; td < NUM_TESTS ; td++){
			cout << endl <<"Check test data nr. " << dec << td+1 << endl << "----------" << endl;
			//read package meta data
			sc_packet to_compare_meta = compare_meta.read();

			ap_uint< 64 > type = output_data.read();

			c_size_t size = output_data.read();
			file_length += 16;


			//if duplicate expect sha1 sum
			if (to_compare_meta.is_duplicate){
				cout << "Found duplicate chunk." << endl;

				file_length += 24;

				//check seperator
				//check type
				if (type != TYPE_FINGERPRINT){
					cout << "Wrong type written to file." << endl;
					cout << "expected: " << TYPE_FINGERPRINT << endl;
					cout << "received: " << type << endl;
					errors++;
				}

				//check size
				if(size != 20){
					cout << "Wrong size written to file." << endl;
					cout << "expected: " << to_compare_meta.size << endl;
					cout << "received: " << size << endl;
					errors++;
				}

				//check hash
				addr_t hash;
				hash.range(63, 0) = output_data.read();
				hash.range(127, 64) = output_data.read();
				ap_uint< 64 > buffer = output_data.read();
				hash.range(159, 128) = buffer.range(31, 0);

				if (hash != td){
					cout << left << "Wrong hash" << endl;
					cout << left << "expected: " << right << hex << setw(W_ADDR/16) << td << endl;
					cout << left << "received: " << right << hex << setw(W_ADDR/16) << hash << endl;

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
					cout << "Wrong type written to file." << endl;
					cout << "expected: " << TYPE_COMPRESS << endl;
					cout << "received: " << type << endl;
					errors++;
				}

				//check size
				if(size != to_compare_meta.size){
					cout << "Wrong size written to file." << endl;
					cout << "expected: " << to_compare_meta.size << endl;
					cout << "received: " << size << endl;
					errors++;
				}

				//check chunk data
				for (int i = 0 ; i < hls::ceil((double) size.to_long()*8 / W_DATA) ; i++){
					c_data_t compare = compare_data.read();
					c_data_t output;
					//build output data
					for (int j = 0 ; j < W_DATA/64 ; j++){
						if (size.to_long() > i*W_DATA/8 + j*8){
							output.range(63 + 64*j, 64*j) = output_data.read();
						} else {
							output.range(63 + 64*j, 64*j) = 0;
						}
					}

					//check
					if(output != compare){
						cout << left << "Wrong chunk data" << endl;
						cout << left << "expected: " << right << hex << compare << endl;
						cout << left << "received: " << right << hex << output << endl;

						errors++;
					}
				}
			}
		}

		c_size_t read_size = output_size.read();
		if (read_size != file_length){
			cout << "Data lost. Output_size does not match the read bytes." << endl;
			errors++;
		}

		end = output_end.read();
	}

	cout << "Finished processing." << endl << endl;

	//Check if output stream is complete
	if (!output_data.empty()){
		cout << "Left over data in output FIFO: " << endl;
		int counter = 0;
		while (!output_data.empty()){
			cout << hex << output_data.read() << endl;
			counter++;
		}
		errors++;
		cout << endl;
		cout << "Number of bytes which were left in FIFO: " << counter << endl;
	}

	if (!compare_data.empty()){
		cout << "Not processed compare data:" << endl;
		int counter = 0;
		while (!compare_data.empty()){
			cout << hex << compare_data.read() << endl;
			counter++;
		}
		errors++;
		cout << "Number of c_data_t chunks left in compare stream: " << counter << endl;
	}


	//Final report
	if (errors == 0){
		cout << " All " << dec << NUM_TESTS << " tests passed." << endl;
	} else {
		cout << "Tests failed." << endl;
	}

	return errors;
}
