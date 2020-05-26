/*
 * This file contains the interface to the bram modules used for the global hash table of the dedup kernels
 */


#include "bram.h"

void bram(bool wren, bool rden,
		bram_packet packet_w,
		bram_packet &packet_r){
#pragma HLS PIPELINE II=1

  static sc_data_t buffer[W_ADDR][SC_ARRAY_SIZE];
#pragma HLS ARRAY_PARTITION variable=buffer type=cyclic factor=NUM_BRAM dim=1
#pragma HLS DEPENDENCE variable=buffer type=inter direction=WAR false

  if (rden){
	  for (int i = 0 ; i < SC_ARRAY_SIZE ; i++ ){
#pragma HLS UNROLL
		  packet_r.data[i] = buffer[packet_r.addr][i];
	  }
  }

  if (wren){
	  for (int i = 0 ; i < SC_ARRAY_SIZE ; i++ ){
#pragma HLS UNROLL
		  buffer[packet_w.addr][i] = packet_w.data[i];
	  }
  }
}
