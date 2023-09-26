/**
 * @file dedup.hpp
 *
 * @brief Function definitions and macros for the dedup pipeline stage
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */


#ifndef DEDUP_HPP
#define DEDUP_HPP

//user includes
#include "bus_def.hpp"
#include "bram.hpp"
#include "top.hpp"
//includes from vitis libraries
#include "hls_stream.h"
#include "sha1.hpp"
#include "hls_math.h"

/*
 * @brief size of the FIFO to the sha1 kernel
 */
#define MSG_BUFF_SIZE (int) (MAX_SMALL_CHUNK_SIZE/32 + 1)

/**
 * @brief Dedup pipeline stage
 *
 * @param meta_in Small chunk meta information
 * @param data_in Stream of small chunk data
 * @param end_in Indicates end of process after each small chunk
 * @param meta_out Updated is_duplicate flag, otherwise same as meta_in
 * @param data_out Same as data_in
 * @param end_out Same as end_in
 */
void dedup(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool > &end_in,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out);

#endif //DEDUP_HPP
