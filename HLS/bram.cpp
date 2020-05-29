/*
 * This file contains the interface to the bram modules used for the global hash table of the dedup kernels
 */


#include "bram.h"

/*
 * bram function implements a hash table with linear probing
 */
void bram(bool wren, bool rden,
		bram_packet packet_w,
		bram_packet &packet_r){
#pragma HLS PIPELINE

	static bram_data buffer[MAX_BRAM_SIZE];
	#pragma HLS ARRAY_PARTITION variable=buffer type=cyclic factor=128
	#pragma HLS DEPENDENCE variable=buffer type=inter direction=WAR false

	if (rden){
	// linear probing: looking for entry that contains data with correct hash value to read from
	  unsigned long int addr = packet_r.addr % MAX_BRAM_SIZE;
	  while (buffer[addr].hash != 0 && buffer[addr].hash != packet_r.addr)
		  ++addr;

	  //read data
	  read_loop_bram:
	  for (int i = 0 ; i < SC_ARRAY_SIZE ; i++ ){
	#pragma HLS UNROLL
		  packet_r.data[i] = buffer[addr].data[i];
	  }
	}

	if (wren){
	//linear probing: looking for next free entry after addr to store data into
	  unsigned long int addr = packet_w.addr % MAX_BRAM_SIZE;
	  while (buffer[addr].hash != 0 && buffer[addr].hash != packet_w.addr)
			  ++addr;

	  //write data
	  write_loop_bram:
	  for (int i = 0 ; i < SC_ARRAY_SIZE ; i++ ){
	#pragma HLS UNROLL
		  buffer[addr].data[i] = packet_w.data[i];
		  buffer[addr].hash =  packet_w.addr;
	  }
	}
}
