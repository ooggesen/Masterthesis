/*
 * @file bram.hpp
 *
 * @brief Function prototypes for the BRAM module, implemented in the dedup pipeline stage
 *
 * @author Ole Oggesen
 */

#ifndef BRAM_HPP
#define BRAM_HPP

#include "bus_def.hpp"

//bug fix for rtl/c simulation
#include "hls_math.h"

#define BRAM_DEPTH ((int) SC_STREAM_SIZE/4) //must be a fraction of SC_STREAM_SIZE

#define MAX_BRAM_SIZE ((int)MAX_FILE_SIZE*8/SMALL_CHUNK_SIZE * SC_STREAM_SIZE/BRAM_DEPTH * 3/4)
//number of small chunk packages the complete BRAM can hold, assumes a duplication rate of one fourth

struct bram_data{
	c_data_t data[BRAM_DEPTH];
	addr_t hash;
};


/*
 * @brief interface to the BRAM for the dedup pipeline stage
 *
 * bram function implements a hash table with linear probing
 * be aware: These functions do not implement bit stuffing with 0, if size is not a multiple of W_DATA * BRAM_DEPTH
 * 	-> stuff the data correctly when passing it to the BRAM
 *
 * if rden and wren are set both to true, then the buffer is initialized
 *
 * @param wren write flag
 * @param rden read flag
 * @param packet_w write data interface
 * @param packet_r read data interface
 * @param size number of bytes to process
 */
void bram(bool wren, bool rden,
		bram_packet packet_w,
		bram_packet &packet_r,
		c_size_t size);
#endif //BRAM_HPP
