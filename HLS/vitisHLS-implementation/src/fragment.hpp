/**
 * @file fragment.hpp
 *
 * @brief Function declaration of fragment function.
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#ifndef FRAGMENT_HPP
#define FRAGMENT_HPP

//user includes
#include "bus_def.hpp"
#include "parsec/rabin.hpp"
#include "top.hpp"
//vitis hls includes
#include "hls_stream.h"
#define __gmp_const const
#include "ap_int.h"
#include "hls_math.h"

/**
 * @brief Segments input file stream into coarse grained chunks (big chunks)
 *
 * @param in 		Input data stream containing file
 * @param size_in 	Size of input file
 * @param end_in 	Indicates end of process after each file
 * @param meta_out	Big chunk meta information
 * @param out		Data stream containing big chunk
 * @end_out			Indicating end of process after each big chunk
 */
void fragment(hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< bc_packet > &meta_out,
		hls::stream< c_data_t > &out,
		hls::stream< bool > &end_out);

#endif //FRAGMENT_HPP
