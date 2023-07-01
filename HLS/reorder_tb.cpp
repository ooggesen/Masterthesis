/*
 * This file contains the testbench and helper functions for testing the reorder kernel of the dedup toolchain.
 */

#include "reorder.hpp"
#include "test_bench.hpp"

using namespace std;

#define NUM_TESTS BUFFER_SIZE_2

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
	generate_test_data(NUM_TESTS, true,  test_meta, test_data, compare_meta, compare_data);

	//shuffle the order of the packages to the reorder stage
	cout << "Shuffle test data." << endl;
	hls::stream< sc_packet > shuffled_meta("shuffeled_meta");
	hls::stream< c_data_t > shuffled_data("shuffeled_data");
	shuffle(test_meta, test_data, shuffled_meta, shuffled_data);

	cout << "Finished generating test data." << endl << endl;
	cout << "Running the reorder kernel." << endl;

	hls::stream< ap_uint< 8 > > output_data("output_data");
	reorder(shuffled_meta, shuffled_data, true, output_data);

	cout << "Checking results." << endl << endl;
	int errors = 0;

	//check header
	ap_uint< 32 > checkbit;
	for (int i = 0 ; i < 4 ; i++){
		checkbit.range(7 + i*8, i*8) = output_data.read();
	}
	if (checkbit != CHECKBIT){
		cout << "Wrong checkbit." << endl;
		errors++;
	}

	ap_uint< 8 > compress_type = output_data.read();
	if (compress_type != COMPRESS_NONE){
		cout << "Wrong compress type." << endl;
		errors++;
	}


	//Check the data stream
	for (int td = 0 ; td < NUM_TESTS ; td++){
		cout << endl <<"Check test data nr. " << dec << td+1 << endl << "----------" << endl;
		//read package meta data
		sc_packet to_compare_meta = compare_meta.read();

		ap_uint< 8 > type = output_data.read();

		c_size_t size;
		for (int i = 0 ; i < W_CHUNK_SIZE/8 ; i++){
			size.range(7 + i*8, i*8) = output_data.read();
		}



		//if duplicate expect sha1 sum
		if (to_compare_meta.is_duplicate){
			cout << "Found duplicate chunk." << endl;
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
			for (int j = 0 ; j < W_ADDR/8 ; j++){
				hash.range(7 + 8*j, 8*j) = output_data.read();
			}

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
				for (int j = 0 ; j < W_DATA/8 ; j++){
					if (size.to_long() > i*W_DATA/8 + j){
						output.range(7 + 8*j, 8*j) = output_data.read();
					} else {
						output.range(7 + 8*j, 8*j) = 0;
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

	cout << "Finished processing." << endl << endl;

	//Check if output stream is complete
	if (!output_data.empty()){
		cout << "Left over data in output FIFO: " << endl;
		int counter = 0;
		while (!output_data.empty()){
			cout << hex << output_data.read() << endl;
			counter++;
		}
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
