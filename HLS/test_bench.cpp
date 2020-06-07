/*
 * This file contains all the helper functions for the testbenches
 */


#include "test_bench.h"
using namespace std;



/*
 * prints out the values of a bus packet type
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
 * Generates sorted test data from the rand() function of the stdlib c library
 *
 * @param num_tests: Number of tests to generate
 * @param test_data: stream with generated test data, sorted by l1 and l2 order
 * @param compare_data: copy of test_data with correctly set duplicate flag. (Use for comparing with results)
 */
void generate_test_data(unsigned num_tests, bool set_duplicate, hls::stream< sc_packet > &test_data, hls::stream< sc_packet > &compare_data){
	srand(time(NULL));

	unsigned l1 = 0, l2 = 0;
	sc_packet last;
	for (int td = 0; td < num_tests; td ++){
		sc_packet bp;

		//increment hash as a filler
		bp.hash = td;

		//in 20 percent of cases create duplicate for test -> output the hash value
		bool is_duplicate = false;
		if (rand() % 101 < 20 && td != 0){
			copy(last, bp);
			bp.hash = td; //convenient for checking in the results

			is_duplicate = true;
		} else {
			for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
				bp.data[i] = rand() % (1 << 31);
			}
			bp.size = rand() % 768 + 256;

			is_duplicate = false;
		}

		//update small chunk positions
		bp.l2_pos = l2;
		bp.l1_pos = l1;
		bool is_last_l2_chunk = false;
		// increment the l1 positions after 2/3 of the average big chunk size if transmitted
		// modified by chance by chance
		if (l2 * SMALL_CHUNK_SIZE >= BIG_CHUNK_SIZE && rand() % 101 < 30) {
			l2 = 0;
			l1++;
			is_last_l2_chunk = true;
		} else {
		//increase l2 position else
			l2++;
		}

		bp.last_l2_chunk = is_last_l2_chunk;
		bp.end = td == num_tests - 1;

		//set duplicate flag if wanted
		if (set_duplicate){
			bp.is_duplicate = is_duplicate;
		} else {
			bp.is_duplicate = false;
		}

		//write data to corresponding stream
		test_data.write(bp);

		bp.is_duplicate = is_duplicate;
		compare_data.write(bp);

		//print out correctly set bus packet
		print_test_data(bp);

		copy(bp, last);
	}
}



/*
 * Shuffles bus packages which are ordered by the l1 and l2 numbers
 * -> Cleans the contend of sorted and moves it so shuffeled stream
 *
 * @param sorted: stream with sorted input
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
