/**
 * @file reorder_buffer.cpp
 *
 * @brief Contains the functions for the buffer, used in the reorder kernel
 *
 * This file exists so the reorder buffer can be tested individually.
 * Data is written and read according to the level 1 and level 2 positions of the chunk in the file.
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "reorder_buffer.hpp"

void write_buffer(sc_packet &meta, c_data_t data[SC_STREAM_SIZE], buffer_cell buffer[][BUFFER_SIZE_2], int &counter){
	//TODO no safety mechanism if cell is already written to
	buffer_cell *bc = &buffer[meta.l1_pos.to_int() % BUFFER_SIZE_1][meta.l2_pos.to_int() % BUFFER_SIZE_2];
	//write data to buffer
	bc->valid = true;
	bc->meta = sc_packet(meta);
	write_data_to_buffer: for (c_size_t i  = 0 ; i < SC_STREAM_SIZE ; i++){
		bc->data[i] = data[i];
	}
	counter++;
}



void read_buffer(l1_pos_t l1, l2_pos_t l2, buffer_cell buffer[][BUFFER_SIZE_2],
		sc_packet &meta_out, c_data_t *data_out, bool &success, int &counter){
	buffer_cell *bc = &buffer[l1.to_int() % BUFFER_SIZE_1][l2.to_int() % BUFFER_SIZE_2];

	//if entry correct return data
	if (bc->valid && bc->meta.l1_pos == l1 && bc->meta.l2_pos == l2){
		meta_out = sc_packet(bc->meta);

		read_data_from_buffer: for (c_size_t i  = 0 ; i < SC_STREAM_SIZE ; i++){
			data_out[i] = bc->data[i];
		}

		bc->valid = false;
		counter--;
		success = true;
	} else {
		success = false;
	}
}

