/*
 * This file contains the testbench for the dedup kernel
 *
 *
 */
#include "dedup.h"
#include <stdlib.h>
#include <string.h>

#define NUM_TESTS 10

using namespace std;
void print_test_data(bus_packet test_data){
	cout << "-----" << endl;

	//cout << SC_ARRAY_SIZE << endl;
	cout << "data: " << endl;
	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
		cout << i + 1 << ": " << hex << test_data.data[i] << endl;
	}
	cout << dec <<"size: " << test_data.size <<  endl;
	cout << "l1 pos: " << test_data.l1_pos << endl;
	cout << "l2 pos: " << test_data.l2_pos << endl;
	cout << "dup: " << test_data.is_duplicate << endl;

	cout << "-----" << endl;
}

int main()
{
	cout << "**********************************" << endl;
	cout << "       Testing dedup kernel       " << endl;
	cout << "**********************************" << endl;

	//file to write the results
	ofstream ofile("../../../output.dat");
	if (!ofile){
		cout << "Can not open file!" <<  endl;
		return 1;
	}


	//reading test data -> skipped for the moment
	//char data[500];
	cout << "Generating input data." << endl;
	//ifstream ifile("./testdata.txt");

	//ifile >> data;
	//create data -> 0
	bus_packet test_data[NUM_TESTS];
	for (int td = 0; td < NUM_TESTS; td ++){
		for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
			test_data[td].data[i] = rand() % (1 << 31);
		}
		test_data[td].size = rand() % 512 + 256;
		test_data[td].l1_pos = 0;
		test_data[td].l2_pos = 0;
		test_data[td].is_duplicate = false;

		print_test_data(test_data[td]);
	}

	//running dedup
	cout << "Starting dedup kernel." << endl;

	ap_uint< 160 > sha1_debug_out = 0;
	hls::stream< ap_uint< 160 > , NUM_TESTS> output_strm("output_strm");
	bool sha1_debug_end = false;
	for (int i = 0; i < NUM_TESTS; i++){
		dedup(test_data[i], sha1_debug_out, sha1_debug_end);
		output_strm.write(sha1_debug_out);
	}
	cout << "Dedup run finished." << endl;

	//check result
	int errors = 0, retval;
	for (int i = 0; i < NUM_TESTS; i++){
		//write sha1
		ofstream tmp("../../../input.dat", ios::out | ios::binary);
		if (!tmp){
			cout << "Can not open file!" << endl;
			return 1;
		}

		for (int j = 0 ; j < SC_ARRAY_SIZE ; i++){ // TODO: this must output file constrained to the size of the input
			tmp << hex << test_data[i].data[j];
		}

		tmp.close();

		//generate sha1sum on file
		system("sha1sum -b ../../../input.dat > ../../../output.golden.dat");

		ap_uint< 160 > sha1_buff  = output_strm.read();

		//check for differences
		retval = system("diff --brief -w ../../../output.dat ../../../output.golden.dat");
		errors += retval;

		if (retval != 0){
			ifstream tmp("../../../output.golden.dat", ios::out | ios::binary);

			string gld;
			getline(tmp, gld);
			cout << "error: " << endl << sha1_buff << endl << gld << endl;

			tmp.close();
		}
	}

	if (errors == 0){
		cout << "Test passed." << endl;
	}else {
		cout << "Test failed." << endl;
		cout << "Calculated SHA1: " << hex << sha1_debug_out << endl;
	}

	//Destruct file streams
	ofile.close();

	return errors;

}
