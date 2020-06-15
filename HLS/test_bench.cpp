/*
 * This file contains all the helper functions for the testbenches
 */


#include "test_bench.hpp"
using namespace std;



/*
 * prints out the values of a small chunk packet type
 *
 * @param test_data: bus packet to print
 */
void print_test_data(sc_packet test_data){
	cout << "-----" << endl;

	cout << "data in hex: " << endl;
	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
		cout << i + 1 << ": " << hex << test_data.data[i] << endl;
	}
	cout << dec <<"size: " << test_data.size <<  endl;
	cout << "hash in hex: " << hex << test_data.hash << endl;
	cout << dec << "l1 pos: " << test_data.l1_pos << endl;
	cout << "l2 pos: " << test_data.l2_pos << endl;
	cout << "dup: " << test_data.is_duplicate << endl;
	cout << "end: " << test_data.end << endl;

	cout << "-----" << endl;
}



/*
 * prints out the values of a big chunk packet type
 *
 * @param test_data: bus packet to print
 */
void print_test_data(bc_packet test_data){
	cout << "-----" << endl;

	cout << "data in hex: " << endl;
	for (int i = 0 ; i < BC_ARRAY_SIZE ; i++){
		cout << i + 1 << ": " << hex << test_data.data[i] << endl;
	}
	cout << dec <<"size: " << test_data.size <<  endl;
	cout << dec << "l1 pos: " << test_data.l1_pos << endl;

	cout << "-----" << endl;
}



/*
 * Generates random test data in a unsigned char FIFO, based on the rand() function of the stdlib library
 *
 * @param num_tests   : expected number of big chunks that can be generated out of generated data -> emphasis on too much data rather then too less
 * @param test_data   : output stream containing the generated data
 * @param compare_data: copy of test data (Use for comparing with the results)
 */
void generate_test_data(unsigned num_tests, hls::stream< ap_uint< 8 > > &test_data, hls::stream< ap_uint< 8 > > &compare_data){
	srand(time(NULL));

	for (int test_nr = 0 ; test_nr < num_tests ; test_nr++){
		for (int byte_pos = 0 ; byte_pos < 1.5*BIG_CHUNK_SIZE/8 ; byte_pos++){
			unsigned char byte = rand() % 256;

			test_data.write(byte);
			compare_data.write(byte);
		}
	}
}



/*
 * Generates big chunk test data, ,sorted by l1 position from the rand() function of the stdlib c library
 *
 * @param num_tests   : number of tests to generate
 * @param test_data   : stream with generated test data, sorted by l1 and l2 order
 * @param compare_data: copy of test_data (Use for comparing with results)
 */
void generate_test_data(unsigned num_tests, hls::stream< bc_packet > &test_data, hls::stream< bc_packet > &compare_data){
	srand(time(NULL));

	unsigned l1 = 0;
	for (int td = 0 ; td < num_tests ; td++){
		bc_packet packet;

		//generating data
		for (int elem = 0 ; elem < BC_ARRAY_SIZE ; elem++){
			for (int seg = 0 ; seg < W_DATA_BIG_CHUNK / 32 ; seg++){
				packet.data[elem].range(31 + 32 * seg, 32 * seg) = rand() % (1 << 31);
			}
		}

		packet.size = BIG_CHUNK_SIZE/8 - 512 + rand() % 1024;
		packet.l1_pos = l1++;

		if (td == num_tests-1){
			packet.end = true;
		}

		//print_test_data(packet);

		//pushing generated data on stream
		test_data.write(packet);
		compare_data.write(packet);
	}
}



/*
 * Generates sorted small chunk test data from the rand() function of the stdlib c library
 *
 * @param num_tests:     Number of tests to generate
 * @param set_duplicate: if true, the duplicate flag is set accordingly, otherwise duplicate is set to false
 * @param test_data:     stream with generated test data, sorted by l1 and l2 order
 * @param compare_data:  copy of test_data with correctly set duplicate flag. (Use for comparing with results)
 */
void generate_test_data(unsigned num_tests, bool set_duplicate, hls::stream< sc_packet > &test_data, hls::stream< sc_packet > &compare_data){
	srand(time(NULL));

	unsigned l1 = 0, l2 = 0;
	sc_packet last;
	for (int td = 0; td < num_tests; td ++){
		sc_packet packet;

		//increment hash as a filler
		packet.hash = td;

		//in 20 percent of cases create duplicate for test -> output the hash value
		bool is_duplicate = false;
		if (rand() % 101 < 20 && td != 0){
			//copy the last value
			packet = sc_packet(last);
			packet.hash = td; //convenient for checking in the results

			is_duplicate = true;
		} else {
			//generating data
			for (int elem = 0 ; elem < SC_ARRAY_SIZE ; elem++){
				for (int seg = 0 ; seg < W_DATA_SMALL_CHUNK / 32 ; seg++){
					packet.data[elem].range(31 + 32 * seg, 32 * seg) = rand() % (1 << 31);
				}
			}
			packet.size = rand() % 24 + SMALL_CHUNK_SIZE/8 - 12; //small variances in size

			is_duplicate = false;
		}

		//update small chunk positions
		packet.l2_pos = l2;
		packet.l1_pos = l1;
		bool is_last_l2_chunk = false;
		// increment the l1 positions after 2/3 of the average big chunk size if transmitted
		// modified by chance
		if (l2 * SMALL_CHUNK_SIZE >= BIG_CHUNK_SIZE && rand() % 101 < 30) {
			l2 = 0;
			l1++;
			is_last_l2_chunk = true;
		} else {
		//increase l2 position else
			l2++;
		}

		packet.last_l2_chunk = is_last_l2_chunk;
		packet.end = td == num_tests - 1;

		//set duplicate flag if wanted
		if (set_duplicate){
			packet.is_duplicate = is_duplicate;
		} else {
			packet.is_duplicate = false;
		}

		//write data to corresponding stream
		test_data.write(packet);

		packet.is_duplicate = is_duplicate;
		compare_data.write(packet);

		//print out correctly set bus packet
		//print_test_data(packet);

		last = sc_packet(packet);
	}
}



/*
 * Shuffles bus packages which are ordered by the l1 and l2 numbers
 * -> Cleans the contend of sorted and moves it so shuffled stream
 *
 * @param sorted   : stream with sorted input
 * @param shuffeled: stream with shuffeled contend of sorted stream
 */
void shuffle(hls::stream< sc_packet > &sorted, hls::stream< sc_packet > &shuffeled){
	hls::stream< sc_packet > buffer;
	while(!sorted.empty() || !buffer.empty()){
		unsigned percent = rand() % 101;
		if (!sorted.empty() && percent < 33){
			shuffeled.write(sorted.read());
		} else if (!buffer.empty() && (percent < 66 || sorted.empty())){
			shuffeled.write(buffer.read());
		} else if (!sorted.empty()){
			buffer.write(sorted.read());
		}
	}
}
