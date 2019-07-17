/*
 * This file contains the helper functions for manipulation of the bus packet data types
 */

#include "bus_def.hpp"

bool is_equal(const c_data_t a[SC_STREAM_SIZE], const c_data_t b[SC_STREAM_SIZE], c_size_t &size){
	check_equality_sc: for (int i = 0 ; i < hls::ceil((double) size.to_long()*8 / W_DATA) ; i++ ){
#pragma HLS PIPELINE II=7
		if (a[i] != b[i])
			return false;
	}

	return true;
}



//Small chunk packet operators initializations

bool operator==(const sc_packet &a, const sc_packet &b){
	if (a.hash != b.hash ||
			a.is_duplicate != b.is_duplicate ||
			a.l1_pos != b.l1_pos ||
			a.l2_pos != b.l2_pos ||
			a.last_l2_chunk != b.last_l2_chunk ||
			a.size != b.size) return false;
	return true;
}



bool operator!=(const sc_packet &a, const sc_packet &b){
	return !(a==b);
}



/*
 * Empty constructor of small chunk interface
 */
sc_packet::sc_packet(){
	hash = 0;
	size = 0;
	l1_pos = 0;
	l2_pos = 0;
	is_duplicate = false;
	last_l2_chunk = false;
}



/*
 * Copy constructor of small chunk interface
 */
sc_packet::sc_packet(const sc_packet &in){
	is_duplicate = in.is_duplicate;
	l1_pos = in.l1_pos;
	l2_pos = in.l2_pos;
	size = in.size;
	hash = in.hash;
	last_l2_chunk = in.last_l2_chunk;
}



//big chunk packet operator initializations

bool operator==(const bc_packet &a, const bc_packet &b){
	if(a.l1_pos != b.l1_pos ||
			a.size != b.size)
		return false;
	return true;
}

bool operator!=(const bc_packet &a, const bc_packet &b){
	return !(a==b);
}



/*
 * Empty constructor of big chunk bus interface
 */
bc_packet::bc_packet(){
	size = 0;
	l1_pos = 0;
}



/*
 * Copy constructor of big chunk interface
 */
bc_packet::bc_packet(const bc_packet &in){
	l1_pos = in.l1_pos;
	size = in.size;
}
