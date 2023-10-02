/*
 * @brief Testbenches for the round robin schedulers
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "test_bench.hpp"
#include "../scheduler.hpp"


#define NUM_TESTS 5 //number of big chunks packets equivalents

using namespace std;

int split_tb(){
	cout << "******************************************************" << endl;
	cout << "     Testing split scheduler from the top module      " << endl;
	cout << "******************************************************" << endl;
	int errors = 0;
	int num_tests_total = NUM_TESTS*35;
	cout << "Generating enough data for at least " << num_tests_total << " small chunk segments." << endl << endl;


	hls::stream< sc_packet > test_meta("test_meta"), compare_meta("compare_meta");
	hls::stream< c_data_t >  test_data("test_data"), compare_data("compare_data");
	hls::stream< bool > test_end("test_end");
	generate_test_data(num_tests_total, true,  test_meta, test_data, test_end, compare_meta, compare_data);


	//splitter
	cout << "Running the scheduler..." << endl;
	hls::stream< sc_packet > 	meta_split[NP_MERGE], meta_buffer[NP_MERGE];
	hls::stream< c_data_t > 	data_split[NP_MERGE], data_buffer[NP_MERGE];
	hls::stream< bool > 		end_split[NP_MERGE], end_buffer[NP_MERGE];

	bool end = false;
	while(!end){
		top_split(test_meta, test_data, test_end, meta_buffer, data_buffer, end_buffer);

		for (int i = 0 ; i < NP_MERGE ; i++){
			if (end_buffer[i].read()){
				end_split[i].write(true);

				end = true;
				break;
			}
			end_split[i].write(false);

			sc_packet meta = meta_buffer[i].read();
			meta_split[i].write(meta);

			for (int d = 0 ; d < meta.size ; d += W_DATA / 8){
				data_split[i].write(data_buffer[i].read());
			}

			if(!meta_buffer[i].empty() || !data_buffer[i].empty()){
				cerr << "Kernel returned more than one small chunk." << endl;
				errors++;
			}
		}
	}

	for (int i = 0 ; i < NP_MERGE ; i++){
		if (!end_buffer[i].empty()){
			end_split[i].write(true);

			if (!end_buffer[i].read()){
				cerr << "Wrong end flag." << endl;
			}
		}
	}

	cout << "Finished run." << endl << endl << "Checking output" << endl;

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
	int errors = 0;
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

	hls::stream< sc_packet > 	meta_unified("meta_unified"), meta_buffer("meta_buffer");
	hls::stream< c_data_t > 	data_unified("data_unified"), data_buffer("data_buffer");
	hls::stream< bool > 		end_unified("end_unified"), end_buffer("end_buffer");

	//assumes split workes fine, tested in separate testbench
	bool end = false;
	while(!end){
		split<sc_packet, c_data_t>(test_meta, test_data, test_end, meta_split, data_split, end_split);
		top_merge(meta_split, data_split, end_split, meta_buffer, data_buffer, end_buffer);

		for (int i = 0 ; i < NP_REFINE ; i ++){
			if (end_buffer.empty())
				continue;

			if (end_buffer.read()){
				end_unified.write(true);

				end = true;
				break;
			}
			end_unified.write(false);

			sc_packet meta = meta_buffer.read();
			meta_unified.write(meta);

			for (int i = 0 ; i < meta.size ; i += W_DATA / 8){
				data_unified.write(data_buffer.read());
			}
		}

		if(!meta_buffer.empty() || !data_buffer.empty() || !end_buffer.empty()){
			cerr << "WARNING: Kernel returned more than two small chunk." << endl;
			errors++;
		}
	}

	cout << "Finished run." << endl << endl << "Checking output" << endl;

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
