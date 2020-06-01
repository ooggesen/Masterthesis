/*
 * This file contains the helper functions for manipulation of the bus data types
 */

#include "bus_def.h"

bool is_equal(sc_data_t a[SC_ARRAY_SIZE], sc_data_t b[SC_ARRAY_SIZE]){
	check_equality:
	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++ ){
#pragma HLS UNROLL
		if (a[i] != b[i]) return false;
	}

	return true;
}


void copy(bus_packet in, bus_packet &out){
#pragma HLS PIPELINE II=1
	out.end = in.end;
	out.is_duplicate = in.is_duplicate;
	out.l1_pos = in.l1_pos;
	out.l2_pos = in.l2_pos;
	out.size = in.size;
	out.hash = in.hash;
	out.last_l2_chunk = in.last_l2_chunk;

	write_out_loop:
	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
#pragma HLS UNROLL
		out.data[i] = in.data[i];
	}
}
