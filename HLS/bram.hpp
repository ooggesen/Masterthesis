#ifndef BRAM_HPP
#define BRAM_HPP

#include "bus_def.hpp"

//bug fix for rtl/c simulation
#include "hls_math.h"

#define MAX_BRAM_SIZE ((int)MAX_FILE_SIZE*8/SMALL_CHUNK_SIZE * 2/3)
//number of small chunk packages the complete BRAM can hold, assumes a duplication rate of one third

#define BRAM_DEPTH ((int) SC_STREAM_SIZE/4) //must be a fraction of SC_STREAM_SIZE
struct bram_data{
	c_data_t data[BRAM_DEPTH];
	addr_t hash;
};

void bram(bool wren, bool rden,
		bram_packet packet_w,
		bram_packet &packet_r,
		c_size_t size);
#endif //BRAM_HPP
