/**
 * @file reorder_buffer.hpp
 *
 * @brief Declarations and macros used for the reorder buffer in the reorder pipeline stage
 *
 * @author Ole Oggesen
 * @bug
 * 		No minimum small chunk size is defined, which might lead to non sufficient buffer space
 */

#ifndef REORDER_BUFFER_HPP
#define REORDER_BUFFER_HPP

//user includes
#include "bus_def.hpp"
#include "top.hpp"
//vitis hls includes
#include "hls_math.h"

//definitions
#define BUFFER_SIZE_1 NP_REFINE //buffer depth for big chunks
#define BUFFER_SIZE_2 (int) (MAX_BIG_CHUNK_SIZE / SMALL_CHUNK_SIZE + 1) //buffer depth for small chunks

/**
 * @brief Data format for the reorder buffer
 */
struct buffer_cell {
	sc_packet meta;
	c_data_t data[SC_STREAM_SIZE];
	bool valid;
};



/**
 * @brief Write interface for the reorder buffer
 */
void write_buffer(sc_packet &meta, c_data_t data[SC_STREAM_SIZE], buffer_cell buffer[][BUFFER_SIZE_2], int &counter);



/**
 * @brief Read interace for the reorder buffer
 */
void read_buffer(l1_pos_t &l1, l2_pos_t &l2, buffer_cell buffer[][BUFFER_SIZE_2], sc_packet &meta_out, c_data_t data_out[], bool &success, int &counter);

#endif //REORDER_BUFFER_HPP
