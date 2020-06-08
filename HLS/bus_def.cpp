/*
 * This file contains the helper functions for manipulation of the bus packet data types
 */

#include "bus_def.hpp"

bool is_equal(const sc_data_t a[SC_ARRAY_SIZE], const sc_data_t b[SC_ARRAY_SIZE]){
#pragma HLS PIPELINE II=1
	check_equality: for (int i = 0 ; i < SC_ARRAY_SIZE ; i++ ){
#pragma HLS UNROLL
		if (a[i] != b[i]) return false;
	}

	return true;
}



//Small chunk packet operators initializations

bool operator==(const sc_packet &a, const sc_packet &b){
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



bool operator!=(const sc_packet &a, const sc_packet &b){
	return !(a==b);
}



sc_packet::sc_packet(){
#pragma HLS PIPELINE II = 1

	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
#pragma HLS UNROLL
		data[i] = 0;
	}

	hash = 0;
	size = 0;
	l1_pos = 0;
	l2_pos = 0;
	is_duplicate = false;
	last_l2_chunk = false;
	end = false;
}

sc_packet::sc_packet(const sc_packet &in){
#pragma HLS PIPELINE II=1
	end = in.end;
	is_duplicate = in.is_duplicate;
	l1_pos = in.l1_pos;
	l2_pos = in.l2_pos;
	size = in.size;
	hash = in.hash;
	last_l2_chunk = in.last_l2_chunk;

	write_out: for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
#pragma HLS UNROLL
		data[i] = in.data[i];
	}
}



//big chunk packet operator initializations

bool operator==(const bc_packet &a, const bc_packet &b){
	if(a.l1_pos != b.l1_pos && a.size != b.size) return false;

	for (int elem = 0 ; elem < BC_ARRAY_SIZE ; elem++){
		if (a.data[elem] != b.data[elem]) return false;
	}
	return true;
}

bool operator!=(const bc_packet &a, const bc_packet &b){
	return !(a==b);
}

bc_packet::bc_packet(){
#pragma HLS PIPELINE II=1
	init_loop: for (int i = 0 ; i < BC_ARRAY_SIZE ; i++){
#pragma HLS UNROLL
		data[i] = 0;
	}
	size = 0;
	l1_pos = 0;
}

bc_packet::bc_packet(const bc_packet &in){
#pragma PIPELINE II=1
	l1_pos = in.l1_pos;
	size = in.size;

	write_out: for (int i = 0 ; i < BC_ARRAY_SIZE ; i++){
	#pragma HLS UNROLL
		data[i] = in.data[i];
	}
}
