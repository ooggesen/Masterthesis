/*
 * This file contains the testbench for the reorder buffer
 */
#include "reorder.h"
#include "reorder_buffer.h"
#include "test_bench.h"

using namespace std;

int reorder_buffer_tb(){
	cout << "**********************************" << endl;
	cout << "      Testing reorder buffer      " << endl;
	cout << "**********************************" << endl;

	cout << "Generating " << BUFFER_SIZE_2 << " tests for the reorder buffer." << endl;


	static buffer_cell buffer[BUFFER_SIZE_1][BUFFER_SIZE_2];

	//Generate test data
	hls::stream< sc_packet > input, compare;
	generate_test_data(BUFFER_SIZE_2, true, input, compare);

	//Save data to buffer
	cout << "Save generated data to buffer." << endl;
	for (int i = 0 ; i < BUFFER_SIZE_2 ; i++){
		sc_packet bp = input.read();
		write_buffer(bp, buffer);
	}
	cout << endl;

	//Check data
	cout << "Check data in buffer." << endl;
	int errors = 0;
	for (int i = 0 ; i < BUFFER_SIZE_2 ; i++){
		cout << "Test data nr. " << i << endl;
		sc_packet out,  bp = compare.read();
		bool success = false;
		read_buffer(bp.l1_pos, bp.l2_pos, buffer, out, success);

		//Check errors
		if (!success){
			cout << i << " Could not find saved data in buffer." << endl;
			print_test_data(bp);
			errors++;
			continue;
		}

		if (!is_equal(out, bp)){
			cout << i << " Wrong data read from buffer." << endl;
			cout << "Expected: " << endl;
			print_test_data(bp);
			cout << "Received from buffer:" << endl;
			print_test_data(out);
			errors++;
			continue;
		}
	}

	cout << "Test passed with " << errors << " errors." << endl;
	return errors;
}
