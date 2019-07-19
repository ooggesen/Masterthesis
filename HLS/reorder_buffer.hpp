#ifndef REORDER_BUFFER_HPP
#define REORDER_BUFFER_HPP

//user includes
#include "bus_def.hpp"
#include "top.hpp"
//vitis hls includes
#include "hls_math.h"

//definitions
#define BUFFER_SIZE_1 (NP * NP_REFINE) //buffer depth for big chunks
#define BUFFER_SIZE_2 ((int) MAX_BIG_CHUNK_SIZE / SMALL_CHUNK_SIZE) //buffer depth for small chunks

struct buffer_cell {
	sc_packet meta;
	c_data_t data[SC_STREAM_SIZE];
	bool valid;
};

//access functions
void write_buffer(sc_packet &meta, c_data_t data[SC_STREAM_SIZE], buffer_cell buffer[][BUFFER_SIZE_2], int &counter);
void read_buffer(l1_pos_t &l1, l2_pos_t &l2, buffer_cell buffer[][BUFFER_SIZE_2], sc_packet &meta_out, c_data_t data_out[], bool &success, int &counter);

#endif //REORDER_BUFFER_HPP
