/*
 * testbenches for the helper functions in the bus_def.h header file
 */

#include "test_bench.h"
using namespace std;

int copy_tb(){
	cout << "**********************************" << endl;
	cout << "     Testing  copy function       " << endl;
	cout << "**********************************" << endl;

	sc_packet source, target;
	source.hash = 1234567890;
	source.is_duplicate = true;
	source.end = true;
	source.l1_pos = 0;
	source.l2_pos = 0;
	source.last_l2_chunk = true;
	source.size = 512;

	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
		source.data[i] = i;
	}

	copy(source, target);

	//check result
	if (is_equal(source, target)){
		cout << "Test passed." << endl;
		return 0;
	} else {
		cout << "Test failed." << endl;
		return 1;
	}
}
