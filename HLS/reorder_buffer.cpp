/*
 * This file contains the functions for the buffer, used in the reorder kernel
 */

#include "reorder_buffer.hpp"

void write_buffer(sc_packet &meta, c_data_t data[SC_STREAM_SIZE], buffer_cell buffer[][BUFFER_SIZE_2]){
	//TODO no safety mechanism if cell is already written to
	buffer_cell *bc = &buffer[meta.l1_pos.to_int() % BUFFER_SIZE_1][meta.l2_pos.to_int() % BUFFER_SIZE_2];
	//write data to buffer
	bc->valid = true;
	bc->meta = sc_packet(meta);
	write_data_to_buffer: for (c_size_t i  = 0 ; i < SC_STREAM_SIZE ; i++){
		bc->data[i] = data[i];
	}
}

void read_buffer(l1_pos_t &l1, l2_pos_t &l2, buffer_cell buffer[][BUFFER_SIZE_2],
		sc_packet &meta_out, c_data_t *data_out, bool &success){
	buffer_cell *bc = &buffer[l1.to_int() % BUFFER_SIZE_1][l2.to_int() % BUFFER_SIZE_2];

	//if entry correct return data
	if (bc->valid && bc->meta.l1_pos == l1 && bc->meta.l2_pos == l2){
		meta_out = sc_packet(bc->meta);

		read_data_from_buffer: for (c_size_t i  = 0 ; i < SC_STREAM_SIZE ; i++){
			data_out[i] = bc->data[i];
		}

		bc->valid = false;
		success = true;
	} else{
		success = false;
	}
}

