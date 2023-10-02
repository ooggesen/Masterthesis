/*
 * @file dedup_tb.cpp
 *
 * @brief Contains dedup testbench
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "test_bench.hpp"
#include "../dedup.hpp"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <time.h>
using namespace std;

#define NUM_TESTS 100

struct test_result_pair{
	sc_packet test;
	bool is_duplicate;
};


/*
 * dedup testbench
 */
int dedup_tb()
{
	cout << "**********************************" << endl;
	cout << "       Testing dedup kernel       " << endl;
	cout << "**********************************" << endl;
	int errors = 0;

	//Generating input data
	cout << "Generating " << NUM_TESTS << " tests for the dedup kernel." << endl;

	hls::stream< sc_packet > test_meta("test_meta"), compare_meta("compare_meta");
	hls::stream< c_data_t >  test_data("test_data"), compare_data("compare_data");
	hls::stream< bool > test_end("test_end");
	generate_test_data(NUM_TESTS, false,  test_meta, test_data, test_end, compare_meta, compare_data);

	//running dedup
	cout << "Starting dedup kernel." << endl;

	hls::stream< sc_packet > out_meta("out_meta"), buffer_meta("buffer_meta");
	hls::stream< c_data_t > out_data("out_data"), buffer_data("buffer_data");
	hls::stream< bool > out_end("out_end"), buffer_end("buffer_end");

	bool end = false;
	while(!end){
		dedup(test_meta, test_data, test_end, buffer_meta, buffer_data, buffer_end);

		if (buffer_end.read()){
			out_end.write(true);

			end = true;
			break;;
		}
		out_end.write(false);

		sc_packet meta = buffer_meta.read();
		out_meta.write(meta);

		for (int i = 0 ; i < meta.size ; i += W_DATA / 8){
			out_data.write(buffer_data.read());
		}

		if(!buffer_meta.empty() || !buffer_data.empty() || !buffer_end.empty()){
			cerr << "WARNING: Kernel returned more than one small chunk." << endl;
			errors++;
		}
	}

	cout << endl << "Dedup run finished." << endl;

	//check results

	cout << "Checking results." << endl;

	for (int i = 0 ; i < NUM_TESTS ; i++){
		if (out_end.read()){
			cerr << "Wrong end flag. Set too early." << endl;
			errors++;
		}

		sc_packet compare = compare_meta.read();
		sc_packet current = out_meta.read();
		if (current.is_duplicate != compare.is_duplicate){
			cerr << left << "Wrong prediction:" << endl;
			cerr << left << "compare data: "  << right << setw(2) << compare.is_duplicate << endl;
			cerr << left << "out_stream: " << right << setw(2) << current.is_duplicate << endl;
			++errors;
		}

		for (int j = 0 ; j < hls::ceil((double) compare.size.to_long()*8 /W_DATA) ; j++){
			if (compare_data.read() != out_data.read()){
				cerr << "Wrong data output." << endl;
				errors++;
			}
		}
	}

	if (!out_end.read()){
		cout << "Wrong end flag." << endl;
		errors++;
	}

	if (errors == 0){
		cout << "Tests passed." << endl;
	}else {
		cout << errors << " out of " << NUM_TESTS <<" tests failed." << endl;
	}

	return errors;

}
