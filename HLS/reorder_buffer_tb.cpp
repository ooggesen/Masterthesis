/*
 * @file reorder_buffer_tb.cpp
 *
 * @brief Contains reorder_buffer testbench
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */


#include "reorder.hpp"
#include "reorder_buffer.hpp"
#include "test_bench.hpp"

using namespace std;

int reorder_buffer_tb(){
	cout << "**********************************" << endl;
	cout << "      Testing reorder buffer      " << endl;
	cout << "**********************************" << endl;

	cout << "Generating " << BUFFER_SIZE_2 << " tests for the reorder buffer." << endl;


	static buffer_cell buffer[BUFFER_SIZE_1][BUFFER_SIZE_2];

	//Generate test data
	hls::stream< sc_packet > test_meta("test_meta"), compare_meta("compare_meta");
	hls::stream< c_data_t > test_data("test_data"), compare_data("compare_data");
	hls::stream< bool > test_end("test_end");
	generate_test_data(BUFFER_SIZE_2, true, test_meta, test_data, test_end, compare_meta, compare_data);

	//Save data to buffer
	cout << "Save generated data to buffer." << endl;
	int counter = 0;
	for (int i = 0 ; i < BUFFER_SIZE_2 ; i++){
		test_end.read();

		sc_packet bp = test_meta.read();
		c_data_t data[SC_STREAM_SIZE];

		//copy data to buffer
		c_size_t j = 0;
		for ( ; j < hls::ceil((double) bp.size.to_long()*8 / W_DATA) ; j++){
			data[j] = test_data.read();
		}

		for ( ; j < SC_STREAM_SIZE ; j++){
			data[j] = 0;
		}

		//write buffer
		write_buffer(bp, data, buffer, counter);
	}
	test_end.read();
	cout << endl;

	//Check data
	cout << "Check data in buffer." << endl;
	int errors = 0;
	for (int i = 0 ; i < BUFFER_SIZE_2 ; i++){
		cout << "Test data nr. " << i << endl;
		sc_packet out,  compare = compare_meta.read();
		bool success = false;
		c_data_t data_buffer[SC_STREAM_SIZE];
		read_buffer(compare.l1_pos, compare.l2_pos, buffer, out, data_buffer, success, counter);

		//Check errors
		if (!success){
			cout << i << " Could not find saved data in buffer." << endl;
			print_test_data(compare);
			errors++;
		}

		if (out != compare){
			cout << i << " Wrong data read from buffer." << endl;
			cout << "Expected: " << endl;
			print_test_data(compare);
			cout << "Received from buffer:" << endl;
			print_test_data(out);
			errors++;
		}

		c_size_t j = 0;
		for ( ; j < hls::ceil((double) out.size.to_long()*8 /W_DATA) ; j++){
			c_data_t compare = compare_data.read();
			if (compare != data_buffer[j]){
				cout << "Wrong data read from buffer." << endl;
				errors++;
			}
		}

		for ( ; j < SC_STREAM_SIZE ; j++){
			if (data_buffer[j] != 0){
				cout << "Wrong bit stuffing."  << endl;
				errors++;
			}
		}
	}

	if (counter != 0){
		cout << "Data lost in buffer" << endl;
		errors++;
	}

	cout << "Test passed with " << errors << " errors." << endl;
	return errors;
}
