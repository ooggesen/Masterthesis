/**
 * @file reorder.hpp
 *
 * @brief contains reorder fucntion declaration.
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#ifndef REORDER_H
#define REORDER_H

//user libraries
#include "bus_def.hpp"
#include "parsec.hpp"
#include "reorder_buffer.hpp"
//vitis libraries
#include "hls_stream.h"
#define __gmp_const const
#include "ap_int.h"
#include "hls_math.h"

/**
 * @brief Assembles output stream in an ordered fashion according to the level 1 and level 2 positional arguments
 *
 * Segments bus data into top level interface data
 *
 * @param meta_in 	Small chunk meta data with duplicate information
 * @param data_in	Small chunk data
 * @param end_in	Indicates end of process after each small chunk.
 * @param size_out	Size of deduplicated file
 * @param data_out	Output data stream
 * @param end_out	Indicates end of process after each file
 */
void reorder(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool > &end_in,
		hls::stream< c_size_t > &size_out,
		hls::stream< ap_uint< 64 > > &data_out,
		hls::stream< bool > &end_out);

#endif //REORDER_H
