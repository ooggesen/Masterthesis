/*
 * @file bram_tb.cpp
 *
 * @brief Contains bram testbench
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include <iostream>

#include "bus_def.hpp"
#include "bram.hpp"
#include "test_bench.hpp"
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
	hls::stream< c_size_t > random_size_stream("random_size_stream");
	int errors = 0;

	cout << "Writing data." << endl;

	srand(time(NULL));
	for (int i = 0 ; i < NUM_TESTS ; i++){
		packet_w.addr = i;
		for (int j = 0 ; j < SC_STREAM_SIZE ; j++){
			packet_w.data[j] = i * j;
		}

		//build a random size
		c_size_t random_size = rand() % (SC_STREAM_SIZE * W_DATA / 8);
		random_size_stream.write(random_size);

		//print_data(packet_w);
		bram(true, false, packet_w, packet_r, random_size);
	}

	cout << "Reading data." << endl;

	for (int i = 0 ; i < NUM_TESTS ; i++){
		packet_r.addr = i;

		c_size_t read_size = random_size_stream.read();
		bram(false, true, packet_w, packet_r, read_size);

		//print_data(packet_r);

		for (int j = 0 ; j < hls::ceil((double) read_size.to_long()*8 / W_DATA) ; j++){
			if (packet_r.data[j] != i * j) errors += 1;
		}
	}

	if (errors == 0){
		cout << "Tests complete without errors." << endl;
	} else {
		cout << "Tests complete with " << dec << errors << " errors." << endl;
	}

	return errors;
}
