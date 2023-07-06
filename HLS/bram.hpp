#ifndef BRAM_HPP
#define BRAM_HPP

#include "bus_def.hpp"
#include "hls_math.h"

#define MAX_BRAM_SIZE 1024//(5 * BIG_CHUNK_SIZE / SMALL_CHUNK_SIZE) //number of small chunk packages the complete BRAM can hold

//TODO change length of data stored in a bram cell
#define BRAM_DEPTH ((int) SC_STREAM_SIZE/4)
struct bram_data{
	c_data_t data[BRAM_DEPTH];
	addr_t hash;
};

void bram(bool wren, bool rden,
		bram_packet packet_w,
		bram_packet &packet_r,
		c_size_t size);
#endif //BRAM_HPP
