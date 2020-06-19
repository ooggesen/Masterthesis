/*
 * This file contains the testbench for the fragment refine function
 * -> this testbench does not test the correct segementation of the big chunk, this is done in the rabin_tb()
 *
 */
#include "test_bench.hpp"
#include "fragment_refine.hpp"

#define NUM_TESTS 2

using namespace std;
int fragment_refine_tb(){
	cout << "**********************************" << endl;
	cout << "  Testing fragment refine kernel  " << endl;
	cout << "**********************************" << endl;

	//Generating input data
	cout << "Generating " << NUM_TESTS << " tests for the fragment refine kernel." << endl << endl;

	hls::stream< bc_packet > test_data, compare_data;
	generate_test_data(NUM_TESTS, test_data, compare_data);

	//running
	cout << "Running the fragment refine kernel." << endl;

	hls::stream< sc_packet > out_stream;
	fragment_refine(test_data, true, out_stream);

	cout << "Test run finished." << endl << endl;

	//Checking
	cout << "Checking the results." << endl;
	int errors = 0;
	unsigned long long l1 = 0, l2 = 0;
	unsigned long long total_l1_size = 0;
	for (int td = 0 ; td < NUM_TESTS ; td++){
		cout << "checking big chunk nr.: " << td << endl << endl;
		//buffer for packets
		bc_packet current_bc = compare_data.read();
		sc_packet current_sc;
		//buffer for metadata
		total_l1_size = 0;
		l2 = 0;
		//buffer for position in big chunk
		int bc_pos = 0;
		do{
			cout << "checking small chunk nr.: " << l2 << "-----" << endl;
			current_sc = out_stream.read();

			//Checking metadata
			if(current_sc.l1_pos != l1 ||
					current_sc.l2_pos != l2){
				cout << "Wrong positional arguments." << endl;
				errors++;
			}

			total_l1_size += current_sc.size.to_long();
			l2++;

			//Checking data
			for (int i = 0 ; i < current_sc.size.to_long() ; i++){
				ap_uint< 8 > sc_byte = current_sc.data[(int)i / (W_DATA_SMALL_CHUNK/8)].range(7 + ((8*i) % W_DATA_SMALL_CHUNK), (8*i) % W_DATA_SMALL_CHUNK);
				ap_uint< 8 > bc_byte = current_bc.data[(int)bc_pos / (W_DATA_BIG_CHUNK/8)].range(7 + ((bc_pos*8) % W_DATA_BIG_CHUNK), (8*bc_pos) % W_DATA_BIG_CHUNK);

				if (sc_byte != bc_byte){
					cout << "Wrong data in fragmented small chunk." << endl;
					cout << "Expected: " << bc_byte << ", received: " << sc_byte << endl;
					errors++;
				}
				bc_pos++;
			}
		}while(!current_sc.last_l2_chunk);

		//Checking integrity of big chunk after fragmentation
		if (total_l1_size != current_bc.size.to_long()){
			cout << "Data was lost in segmentation process. Small chunks summed up to " << total_l1_size << ", but expected " << current_bc.size.to_long() << endl;
			errors++;
		}

		l1++;
		cout << endl;
	}

	if (errors > 0){
		cout << "Tests failed with " << errors << " errors." << endl;
	} else {
		cout << "Tests passed." << endl;
	}

	return errors;
}
