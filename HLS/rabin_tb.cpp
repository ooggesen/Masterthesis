/*
 * This file contains the testbench for the rabin fingerprint algorithm
 */
#include "test_bench.hpp"
#include "parsec/rabin.hpp"

#define NUM_TESTS 2

using namespace std;
int rabin_tb(){
	cout << "**********************************" << endl;
	cout << "   Testing rabin fingerprinting   " << endl;
	cout << "**********************************" << endl;

	//Generating input data
	cout << "Generating " << NUM_TESTS << " tests for the rabin fingerprinting algorithm." << endl << endl;

	//generate big chunk test data
	hls::stream< bc_packet > test_data, compare_data;
	generate_test_data(NUM_TESTS, test_data, compare_data);

	//initialize arrays for rabin fingerprint
	cout << "Initializing the rabintab and rabinwintab arrays." <<  endl << endl;
	int winlen = 0;
	unsigned rabintab[256], rabinwintab[256];
	rabininit(winlen, rabintab, rabinwintab);

	//run rabin seg on big chunk data
	int errors = 0;
	cout << "Running rabin fingerprint on big chunk data." << endl << endl;
	hls::stream< sc_packet > results;
	for (int t_nr = 0 ; t_nr < NUM_TESTS ; t_nr++){
		cout << "Big chunk nr.: " << t_nr << endl;

		int counter = 0;
		bc_packet packet = test_data.read();
		sc_packet out;
		while(packet.size.to_long() > 0){
			cout << "." << flush;

			//run rabin fingerprint
			rabinseg_bc_packet(packet, out, winlen, rabintab, rabinwintab);

			if(out.size.to_long() == 0){
				cout << "Segmentation fail: Empty package." << endl;
				errors++;
			}

			//update variables
			results.write(out);
			out = sc_packet();
			counter++;
		}

		cout << endl << dec << counter << " small chunks generated from this big chunk." << endl;
	}

	//Checking the results
	cout << endl << "Checking results." << endl << endl;

	//Since rabinseg should segment the data always at the same locations, perform a second run with rabinseg and compare the differences
	rabininit(winlen, rabintab, rabinwintab);
	for (int t_nr = 0 ; t_nr < NUM_TESTS ; t_nr++){
		int counter = 0;
		bc_packet packet = compare_data.read();
		sc_packet out;
		while(packet.size.to_int() > 0){
			cout << "Running check on bc l1 pos: " << t_nr << ", sc l2 pos: " << counter << endl;


			rabinseg_bc_packet(packet, out, winlen, rabintab, rabinwintab);

			sc_packet compare = results.read();
			if (out != compare){
				cout << "Differeng segmtation on the second run." << endl;
				//print_test_data(compare);
				errors++;
			}

			counter++;
		}
	}

	if (errors == 0){
		cout << "Tests passed." << endl << endl;
	} else {
		cout << "Tests failed." << endl << endl;
	}

	return errors;
}
