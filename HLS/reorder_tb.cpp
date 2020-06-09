/*
 * This file contains the testbench and helper functions for testing the reorder kernel of the dedup toolchain.
 */

#include "reorder.hpp"
#include "test_bench.hpp"

using namespace std;

#define NUM_TESTS 100

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


	hls::stream< sc_packet > test_data;
	hls::stream< sc_packet > compare_data;
	generate_test_data(NUM_TESTS, true,  test_data, compare_data);

	//shuffle the order of the packages to the reorder stage
	cout << "Shuffle test data." << endl;
	hls::stream< sc_packet > shuffeled;
	shuffle(test_data, shuffeled);

	cout << "Finished generating test data." << endl;
	cout << "Running the reorder kernel." << endl;

	hls::stream< ap_uint< 8 > > output_data;
	reorder(shuffeled, output_data);

	cout << "Checking results." << endl;
	int errors = 0;

	//check header
	ap_uint< 32 > checkbit;
	for (int i = 0 ; i < 4 ; i++){ //TODO assume MSB first to send
		checkbit.range(31 - i*8, 24 - i*8) = output_data.read();
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
	sc_packet last_compare = sc_packet();
	for (int td = 0 ; td < NUM_TESTS ; td++){
		cout << endl <<"Check test data nr. " << td+1 << endl << "----------" << endl;
		//check chunk data
		sc_packet to_compare = compare_data.read();

		//if duplicate expect sha1 sum
		if (!last_compare.end){
			if (to_compare.is_duplicate){
				//check seperator
				//check type
				ap_uint< 8 > type = output_data.read();
				if (type != TYPE_FINGERPRINT){
					cout << "Wrong type written to file." << endl;
					cout << "expected: " << TYPE_FINGERPRINT << endl;
					cout << "received: " << type << endl;
					errors++;
				}

				//check size
				c_size_t size;
				for (int i = 0 ; i < W_CHUNK_SIZE / 8 ; i++){
					size.range(W_CHUNK_SIZE-1 - i*8, W_CHUNK_SIZE-8 - i*8) = output_data.read();
				}
				if(size != to_compare.size){
					cout << "Wrong size written to file." << endl;
					cout << "expected: " << to_compare.size << endl;
					cout << "received: " << size << endl;
					errors++;
				}

				//check hash
				addr_t hash;
				for (int j = 0 ; j < W_ADDR / 8 ; j++){
					hash.range(W_ADDR-1 - 8*j, W_ADDR-8 - 8*j) = output_data.read();
				}

				if (hash != td){
					cout << left << "Wrong hash" << endl;
					cout << left << "expected: " << right << hex << setw(W_ADDR/16) << td << endl;
					cout << left << "received: " << right << hex << setw(W_ADDR/16) << hash << endl;

					errors++;
				}
			} else {
			//if unique chunk expect chunk as output

				//check seperator
				//check type
				ap_uint< 8 > type = output_data.read();
				if (type != TYPE_COMPRESS){
					cout << "Wrong type written to file." << endl;
					cout << "expected: " << TYPE_COMPRESS << endl;
					cout << "received: " << type << endl;
					errors++;
				}

				//check size
				c_size_t size;
				for (int i = 0 ; i < W_CHUNK_SIZE / 8 ; i++){
					size.range(W_CHUNK_SIZE-1 - i*8, W_CHUNK_SIZE-8 - i*8) = output_data.read();
				}
				if(size != to_compare.size){
					cout << "Wrong size written to file." << endl;
					cout << "expected: " << to_compare.size << endl;
					cout << "received: " << size << endl;
					errors++;
				}

				//check chunk data
				sc_packet packet;
				for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
					for (int j = 0 ; j < W_DATA_SMALL_CHUNK / 8 ; j++){
						packet.data[i].range( W_DATA_SMALL_CHUNK-1 -j*8, W_DATA_SMALL_CHUNK-8 - 8*j) = output_data.read();
					}
				}

				//compare data to compare stream
				for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
					if (packet.data[i] !=  to_compare.data[i]){
						cout << left << "Wrong chunk data" << endl;
						cout << left << "expected: " << right << hex << to_compare.data[i] << endl;
						cout << left << "received: " << right << hex << packet.data[i] << endl;

						errors++;
					}
				}
			}

			//copy the current to the last small chunk buffer
			last_compare = sc_packet(to_compare);
		}
	}

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

	//Final report
	if (errors == 0){
		cout << " All " << NUM_TESTS << " tests passed." << endl;
	} else {
		cout << "Tests failed." << endl;
	}

	return errors;
}
