/*
 * @brief Testbenches for the round robin schedulers
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "../scheduler.hpp"
#include "test_bench.hpp"

#define NUM_TESTS 2 //number of big chunks packets equivalents

using namespace std;

int split_tb(){
	cout << "******************************************************" << endl;
	cout << "     Testing split scheduler from the top module      " << endl;
	cout << "******************************************************" << endl;

	int num_tests_total = NUM_TESTS*35;
	cout << "Generating enough data for at least " << num_tests_total << " small chunk segments." << endl << endl;


	hls::stream< sc_packet > test_meta("test_meta"), compare_meta("compare_meta");
	hls::stream< c_data_t >  test_data("test_data"), compare_data("compare_data");
	hls::stream< bool > test_end("test_end");
	generate_test_data(num_tests_total, true,  test_meta, test_data, test_end, compare_meta, compare_data);


	//splitter
	cout << "Running the scheduler..." << endl;
	hls::stream< sc_packet > 	meta_split[NP_MERGE];
	hls::stream< c_data_t > 	data_split[NP_MERGE];
	hls::stream< bool > 		end_split[NP_MERGE];

	top_split(test_meta, test_data, test_end, meta_split, data_split, end_split);

	cout << "Finished run." << endl << endl << "Checking output" << endl;

	int errors = 0;
	for ( int num_test = 0 ; num_test < num_tests_total ; num_test++ ){
		bool end = end_split[num_test % NP_MERGE].read();
		sc_packet meta = meta_split[num_test % NP_MERGE].read();

		if (end && !compare_meta.empty()){
			cout << "Mismatch in end flag." << endl;
			errors++;
		}

		if (meta != compare_meta.read()){
			cout << "Mismatch in meta." << endl;
			errors++;
		}

		for (c_size_t i = 0 ; i < meta.size ; i += W_DATA/8){
			c_data_t is, should;
			is = data_split[num_test % NP_MERGE].read();
			should = compare_data.read();
			if (is != should){
				cout << "Mismatch in data." << endl;
				errors++;
			}
		}
	}

	for (int end_n = 0 ; end_n < NP_MERGE ; end_n++ ){
		if (!end_split[end_n].read()){
			cout << "Mismatch in end flag. " << endl;
			errors++;
		}
	}

	return errors;
}



/*
 * @brief tests the merge function used in top() function.
 */
int merge_tb(){
	cout << "******************************************************" << endl;
	cout << "     Testing the schedulers from the top module       " << endl;
	cout << "******************************************************" << endl;

	int num_tests_total = NUM_TESTS*35;
	cout << "Generating enough data for at least " << num_tests_total << " small chunk segments." << endl << endl;


	hls::stream< sc_packet > test_meta("test_meta"), compare_meta("compare_meta");
	hls::stream< c_data_t >  test_data("test_data"), compare_data("compare_data");
	hls::stream< bool > test_end("test_end");
	generate_test_data(num_tests_total, true,  test_meta, test_data, test_end, compare_meta, compare_data);


	//splitter
	cout << "Running the scheduler..." << endl;
	hls::stream< sc_packet > 	meta_split[NP_MERGE];
	hls::stream< c_data_t > 	data_split[NP_MERGE];
	hls::stream< bool > 		end_split[NP_MERGE];

	hls::stream< sc_packet > 	meta_unified("meta_unified");
	hls::stream< c_data_t > 	data_unified("data_unified");
	hls::stream< bool > 		end_unified("end_unified");

	//assumes split workes fine, tested in separate testbench
	split<sc_packet, c_data_t>(test_meta, test_data, test_end, meta_split, data_split, end_split);

	top_merge(meta_split, data_split, end_split, meta_unified, data_unified, end_unified);

	cout << "Finished run." << endl << endl << "Checking output" << endl;

	int errors = 0;
	for (int num_test = 0 ; num_test < num_tests_total ; num_test++){
		sc_packet meta = meta_unified.read();
		bool end = end_unified.read();

		if (end){
			cout << "Mismatch in end flag." << endl;
			errors++;
		}

		if (meta != compare_meta.read()){
			cout << "Mismatch in meta data." << endl;
			errors++;
		}


		for (c_size_t i = 0 ; i < meta.size ; i += W_DATA/8){
			c_data_t is, should;
			is = data_unified.read();
			should = compare_data.read();
			if (is != should){
				cout << "Mismatch in data." << endl;
				errors++;
			}
		}
	}

	if (!end_unified.read()){
		cout << "Mismatch in end flag." << endl;
		errors++;
	}

	return errors;
}
