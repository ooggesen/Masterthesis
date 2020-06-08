/*
 * This file contains the functions for the buffer, used in the reorder kernel
 */

#include "reorder_buffer.hpp"

void write_buffer(sc_packet &to_write, buffer_cell buffer[][BUFFER_SIZE_2]){
	//TODO no safety mechanism if cell is already written to
	buffer_cell *bc = &buffer[to_write.l1_pos.to_int() % BUFFER_SIZE_1][to_write.l2_pos.to_int() % BUFFER_SIZE_2];
	//write data to buffer
	bc->valid = true;
	bc->data = sc_packet(to_write);
}

void read_buffer(l1_pos_t &l1, l2_pos_t &l2, buffer_cell buffer[][BUFFER_SIZE_2], sc_packet &out, bool &success){
	buffer_cell *bc = &buffer[l1.to_int() % BUFFER_SIZE_1][l2.to_int() % BUFFER_SIZE_2];

	//if entry correct return data
	if (bc->valid && bc->data.l1_pos == l1 && bc->data.l2_pos == l2){
		out = sc_packet(bc->data);
		bc->valid = false;
		success = true;
	} else{
		success = false;
	}
}

