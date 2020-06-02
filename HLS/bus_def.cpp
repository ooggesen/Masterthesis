/*
 * This file contains the helper functions for manipulation of the bus data types
 */

#include "bus_def.h"

bool is_equal(sc_data_t a[SC_ARRAY_SIZE], sc_data_t b[SC_ARRAY_SIZE]){
#pragma HLS PIPELINE II=1
	check_equality:
	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++ ){
#pragma HLS UNROLL
		if (a[i] != b[i]) return false;
	}

	return true;
}

bool is_equal(bus_packet a, bus_packet b){
#pragma HLS ARRAY_PARTITION variable=a.data type=complete
#pragma HLS ARRAY_PARTITION variable=b.data type=complete
#pragma HLS PIPELINE II=1

	if (a.end != b.end &&
			a.hash != b.hash &&
			a.is_duplicate != b.is_duplicate &&
			a.l1_pos != b.l1_pos &&
			a.l2_pos != b.l2_pos &&
			a.last_l2_chunk != b.last_l2_chunk &&
			a.size != b.size) return false;
	return is_equal(a.data, b.data);
}


void copy(bus_packet &in, bus_packet &out){
	/*
#pragma HLS INTERFACE mode=m_axi port=in.data
#pragma HLS INTERFACE mode=s_axilite port=in.last_l2_chunk bundle=bools
#pragma HLS INTERFACE mode=s_axilite port=in.is_duplicate bundle=bools
#pragma HLS INTERFACE mode=s_axilite port=in.end bundle=bools
#pragma HLS INTERFACE mode=s_axilite port=in.l1_pos bundle=positions
#pragma HLS INTERFACE mode=s_axilite port=in.l2_pos bundle=positions
#pragma HLS INTERFACE mode=s_axilite port=in.size
#pragma HLS INTERFACE mode=s_axilite port=in.hash
*/

#pragma HLS ARRAY_PARTITION variable=in.data type=complete
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
