/*
 * This file contains the testbench for the bram module
 */

#include <iostream>

#include "bus_def.hpp"
#include "bram.hpp"
using namespace std;

#define NUM_TESTS 200

void print_data(bram_packet data){
	cout << "------" << endl;
	cout << "data: " << endl;

	for (int i = 0 ; i < SC_STREAM_SIZE ; i++){
		cout << hex << data.data[i];
	}
	cout << endl;

	cout << "addr: " << endl;
	cout << data.addr << endl;
}


int bram_tb(){
	cout << "**********************************" << endl;
	cout << "       Testing bram module        " << endl;
	cout << "**********************************" << endl;

	bram_packet packet_w, packet_r;
	int errors = 0;

	cout << "Writing data:" << endl;

	for (int i = 0 ; i < NUM_TESTS ; i++){
		packet_w.addr = i;
		for (int j = 0 ; j < SC_STREAM_SIZE ; j++){
			packet_w.data[j] = i * j;
		}

		//print_data(packet_w);
		bram(true, false, packet_w, packet_r);
	}

	cout << "Reading data:" << endl;

	for (int i = 0 ; i < NUM_TESTS ; i++){
		packet_r.addr = i;

		bram(false, true, packet_w, packet_r);

		//print_data(packet_r);

		for (int j = 0 ; j < SC_STREAM_SIZE ; j++){
			if (packet_r.data[j] != i * j) errors += 1;
		}
	}

	if (errors == 0){
		cout << "Tests complete without errors." << endl;
	} else {
		cout << "Tests completet with " << dec << errors << " errors." << endl;
	}

	return errors;
}
