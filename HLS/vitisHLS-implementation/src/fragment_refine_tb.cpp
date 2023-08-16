/*
 * @file fragment_refine_tb.cpp
 *
 * @brief Contains fragment_refine testbench
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "test_bench.hpp"
#include "fragment_refine.hpp"

#define NUM_TESTS 10

using namespace std;
int fragment_refine_tb(){
	cout << "**********************************" << endl;
	cout << "  Testing fragment refine kernel  " << endl;
	cout << "**********************************" << endl;

	//Generating input data
	cout << "Generating " << NUM_TESTS << " big chunk for segmentation with the fragment refine kernel." << endl << endl;

	hls::stream< bc_packet > test_meta("test_meta"), compare_meta("compare_meta");
	hls::stream< c_data_t > test_data("test_data");
	hls::stream< ap_uint< 8 > > compare_data("compare_data");
	hls::stream< bool > test_end("test_end");
	generate_test_data(NUM_TESTS, test_meta, test_data, test_end, compare_meta, compare_data);

	//running
	cout << "Running the fragment refine kernel." << endl;

	hls::stream< sc_packet > out_meta("out_meta");
	hls::stream< c_data_t > out_data("out_data");
	hls::stream< bool > out_end("out_end");
	fragment_refine(test_meta, test_data, test_end, out_meta, out_data, out_end);

	cout << "Test run finished." << endl << endl;

	//Checking
	cout << "Checking the results." << endl;
	int errors = 0;
	unsigned long long l1 = 0, l2 = 0;
	unsigned long long total_l1_size = 0;
	while (!compare_meta.empty()){
		cout << "checking big chunk nr.: " << l1 << endl << endl;

		//buffer for packets
		bc_packet current_bc = compare_meta.read();
		sc_packet current_sc;
		c_data_t bc_buffer;
		c_data_t sc_buffer;

		//buffer for metadata
		total_l1_size = 0;
		l2 = 0;
		int bc_pos = 0;
		//buffer for position in big chunk
		do{
			cout << "checking small chunk nr.: " << l2 << "-----" << endl;

			if (out_end.read()){
				cout << "Wrong end flag." << endl;
				errors++;
			}

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
			for (int i = 0 ; i < hls::ceil((double) current_sc.size.to_long()*8 / W_DATA) ; i++){
				sc_buffer = out_data.read();
				c_data_t compare_buffer;

				//build compare c_data_t
				for (int j = 0 ; j < W_DATA/8 ; j++){
					if (current_sc.size.to_long()> i*W_DATA/8 + j)
						compare_buffer.range(7 + 8*j, 8*j) = compare_data.read();
					else
						compare_buffer.range(7 + 8*j, 8*j) = 0;
				}
				if (sc_buffer != compare_buffer){
					cout << "Wrong data detected, at transmission of " << (double) i/hls::ceil((double) current_sc.size.to_long()*8 / W_DATA)*100 << "% of small chunk." << endl;
					//cout << "Expected: " << bc_buffer.range(7 + 8*bc_pos, 8*bc_pos) << ", received: " << sc_buffer.range(7 + 8*j, 8*j) << endl;
					errors++;
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

	if(!out_end.read()){
		cout << "Wrong  end flag at end of process." << endl;
		errors++;
	}


	if (!compare_data.empty()){
		cout << "Data lost at end of file." << endl;
		errors++;
	}

	if (errors > 0){
		cout << "Tests failed with " << errors << " errors." << endl;
	} else {
		cout << "Tests passed." << endl;
	}

	return errors;
}
