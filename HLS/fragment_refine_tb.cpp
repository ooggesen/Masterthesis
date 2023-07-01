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

	hls::stream< bc_packet > test_meta, compare_meta;
	hls::stream< ap_uint < 8 > > test_data, compare_data;
	generate_test_data(NUM_TESTS, test_meta, test_data, compare_meta, compare_data);

	//running
	cout << "Running the fragment refine kernel." << endl;

	hls::stream< sc_packet > out_meta;
	hls::stream< c_data_t > out_data;
	fragment_refine(test_meta, test_data, true, out_meta, out_data);

	cout << "Test run finished." << endl << endl;

	//Checking
	cout << "Checking the results." << endl;
	int errors = 0;
	unsigned long long l1 = 0, l2 = 0;
	unsigned long long total_l1_size = 0;
	while (!out_meta.empty()){
		cout << "checking big chunk nr.: " << l1 << endl << endl;
		//buffer for packets
		bc_packet current_bc = compare_meta.read();
		sc_packet current_sc;
		//buffer for metadata
		total_l1_size = 0;
		l2 = 0;
		//buffer for position in big chunk
		do{
			cout << "checking small chunk nr.: " << l2 << "-----" << endl;
			current_sc = out_meta.read();

			//Checking metadata
			if(current_sc.l1_pos != l1 ||
					current_sc.l2_pos != l2){
				cout << "Wrong positional arguments." << endl;
				errors++;
			}

			total_l1_size += current_sc.size.to_long();
			l2++;

			//Checking data
			for (c_size_t i = 0 ; i < hls::ceil((double) current_sc.size.to_long()*8 / W_DATA) ; i++){
				c_data_t sc_byte = out_data.read();
				for (int j = 0 ; j < W_DATA/8 ; j++){
					if (current_sc.size * 8 > i*W_DATA + j*8){
						ap_uint< 8 > bc_byte = compare_data.read();
						if (sc_byte.range(7 + 8*j, 8*j) != bc_byte){
							cout << "Wrong data in fragmented small chunk." << endl;
							cout << "Expected: " << bc_byte << ", received: " << sc_byte.range(7 + 8*j, 8*j) << endl;
							errors++;
						}
					} else {
						if (sc_byte.range(7 + 8*j, 8*j) != 0){
							cout << "Wrong data stuffing in small chunk." << endl;
						}
					}
				}
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


	if (!compare_data.empty()){
		cout << "Data lost at end of file. Data: " << endl;
		while (!compare_data.empty()){
			cout << compare_data.read() << endl;
		}
		errors++;
	}

	if (errors > 0){
		cout << "Tests failed with " << errors << " errors." << endl;
	} else {
		cout << "Tests passed." << endl;
	}

	return errors;
}
