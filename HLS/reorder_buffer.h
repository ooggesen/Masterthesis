#ifndef REORDER_BUFFER_H
#define REORDER_BUFFER_H

#include "bus_def.h"

//definitions
#define BUFFER_SIZE_1 2 //buffer depth for big chunks
#define BUFFER_SIZE_2 (BIG_CHUNK_SIZE / SMALL_CHUNK_SIZE / 10) //buffer depth for small chunk

struct buffer_cell {
	sc_packet data;
	bool valid;
};

//access functions
void write_buffer(sc_packet &to_write, buffer_cell buffer[][BUFFER_SIZE_2]);
void read_buffer(l1_pos_t &l1, l2_pos_t &l2, buffer_cell buffer[][BUFFER_SIZE_2], sc_packet &out, bool &success);


#endif //REORDER_BUFFER_H
