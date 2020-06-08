/*
 * This file contains the testbench for the rabin fingerprint algorithm
 */
#include "test_bench.hpp"
#include "parsec/rabin.hpp"

#define NUM_TESTS 1

using namespace std;
int rabin_tb(){
	cout << "**********************************" << endl;
	cout << "   Testing rabin fingerprinting   " << endl;
	cout << "**********************************" << endl;

	//Generating input data
	cout << "Generating " << NUM_TESTS << " tests for the rabin fingerprinting algorithm." << endl;

	//generate big chunk test data
	hls::stream< bc_packet > test_data, compare_data;
	generate_test_data(NUM_TESTS, test_data, compare_data);

	//initialize arrays for rabin fingerprint
	cout << "Initializing the rabintab and rabinwintab arrays." <<  endl;
	int winlen = 0;
	ap_uint< 32 > rabintab[256], rabinwintab[256];
	rabininit(winlen, rabintab, rabinwintab);

	//run rabin seg on big chunk data
	cout << "Running rabin fingerprint on big chunk data." << endl;
	hls::stream< sc_packet > results;
	for (int t_nr = 0 ; t_nr < NUM_TESTS ; t_nr++){
		cout << "Big chunk nr.: " << t_nr << endl;

		int counter = 0;
		bc_packet packet = test_data.read();
		sc_packet out;
		while(packet.size > 0){
			cout << ".";
			rabinseg_bc_packet(packet, out, winlen, rabintab, rabinwintab);
			results.write(out);
			counter++;
		}

		cout << endl << counter << "small chunk generated from this big chunk." << endl;
	}

	//Checking the results
	cout << endl << "Checking results." << endl;

	return 0;
}
