/*
 * testbenches for the helper functions in the bus_def.h header file
 */

#include "test_bench.hpp"
using namespace std;

int copy_tb(){
	cout << "**********************************" << endl;
	cout << "     Testing  copy function       " << endl;
	cout << "**********************************" << endl;

	sc_packet source, target;
	source.hash = 1234567890;
	source.is_duplicate = true;
	source.l1_pos = 0;
	source.l2_pos = 0;
	source.last_l2_chunk = true;
	source.size = 512;

	target = sc_packet(source);

	//check result
	if (source == target){
		cout << "Test passed." << endl;
		return 0;
	} else {
		cout << "Test failed." << endl;
		return 1;
	}
}

int is_equal_tb(){
	cout << "**************************************" << endl;
	cout << "     Testing  is_equal function       " << endl;
	cout << "**************************************" << endl;

	c_data_t data[SC_STREAM_SIZE];
	for (int j = 0 ; j < SC_STREAM_SIZE ; j++){
		data[j] = j * j;
	}

	int errors = 0;
	//build a random size
	c_size_t random_size = rand() % (SC_STREAM_SIZE * W_DATA / 8);

	if (!is_equal(data, data, random_size)){
		cout << "Not equal when should be equal" << endl;
		errors++;
	}

	c_data_t empty_data[SC_STREAM_SIZE];
	if (is_equal(data, empty_data, random_size)){
		cout << "Equal when should be not equal." << endl;
		errors++;
	}

	if (errors == 0){
		cout << "Tests complete without errors." << endl;
	} else {
		cout << "Tests complete with " << dec << errors << " errors." << endl;
	}

	return errors;
}
