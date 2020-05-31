/*
 * This file contains the testbench and helper functions for testing the reorder kernel of the dedup toolchain.
 */

#include "reorder.h"
#include "test_bench.h"

#include <iostream>
using namespace std;
#include <time.h>
#include <stdlib.h>

#define NUM_TESTS 100

struct test_result_pair{
	bus_packet test;
	addr_t hash;
};

/*
 * reorder tb function
 */
int main(){
	cout << "**********************************" << endl;
	cout << "       Testing reorder kernel       " << endl;
	cout << "**********************************" << endl;

	//Generating input data
	cout << "Generating " << NUM_TESTS << " tests for the reorder kernel." << endl;

	srand(time(NULL));

	hls::stream< bus_packet > test_data;
	hls::stream< bus_packet > compare_data;
	unsigned l1 = 0, l2 = 0;;
	bus_packet last;
	for (int td = 0; td < NUM_TESTS; td ++){
		bus_packet bp;

		//increment hash as a filler
		bp.hash = td;

		//in 20 percent of cases create duplicate for test -> output the hash value
		if (rand() % 101 < 20 && td != 0){
			copy(last, bp);
			bp.hash = td; //convenient for checking in the results

			bp.is_duplicate = true;
		} else {
			for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
				bp.data[i] = rand() % (1 << 31);
			}
			bp.size = rand() % 1024 + 256;

			bp.is_duplicate = false;
		}

		// 1 big chunk is 1 to 3 small chunks
		bp.l2_pos = l2;
		bp.l1_pos = l1;
		bool is_last_l2_chunk = false;
		if (l2 >= bp.size / W_DATA_SMALL_CHUNK) {
			l2 = 0;
			l1++;
			is_last_l2_chunk = true;
		} else {
			l2++;
		}

		bp.last_l2_chunk = is_last_l2_chunk;
		bp.end = td == NUM_TESTS - 1;

		//print bus packet
		print_test_data(bp);

		//add it to the test data queue
		test_data.write(bp);
		compare_data.write(bp);

		copy(bp, last);
	}

	cout << "Finished generating test data." << endl;
	cout << "Running the reorder kernel." << endl;

	hls::stream< ap_uint< 8 > > output_data;
	reorder(test_data, output_data);

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


	for (int td = 0 ; td < NUM_TESTS ; td++){
		cout << endl <<"Check test data nr. " << td+1 << endl << "----------" << endl;
		//check chunk data
		bus_packet to_compare = compare_data.read();

		//if duplicate expect sha1 sum
		if (to_compare.is_duplicate){
			//check seperator
			ap_uint< 8 > type = output_data.read();
			if (type != TYPE_FINGERPRINT){
				cout << "Wrong type written to file." << endl;
				cout << "expected: " << TYPE_FINGERPRINT << endl;
				cout << "received: " << type << endl;
				errors++;
			}

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
			ap_uint< 8 > type = output_data.read();
			if (type != TYPE_COMPRESS){
				cout << "Wrong type written to file." << endl;
				cout << "expected: " << TYPE_COMPRESS << endl;
				cout << "received: " << type << endl;
				errors++;
			}

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
			bus_packet bp;
			for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
				for (int j = 0 ; j < W_DATA_SMALL_CHUNK / 8 ; j++){
					bp.data[i].range( W_DATA_SMALL_CHUNK-1 -j*8, W_DATA_SMALL_CHUNK-8 - 8*j) = output_data.read();
				}
			}

			//compare data to compare stream
			for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
				if (bp.data[i] !=  to_compare.data[i]){
					cout << left << "Wrong chunk data" << endl;
					cout << left << "expected: " << right << hex << to_compare.data[i] << endl;
					cout << left << "received: " << right << hex << bp.data[i] << endl;

					errors++;
				}
			}
		}
	}

	if (errors == 0){
		cout << " ALl " << NUM_TESTS << " tests passed." << endl;
	} else {
		cout << "Tests failed." << endl;
	}

	return errors;
}
