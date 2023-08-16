/**
 * @file fragment_refine.hpp
 *
 * @brief Contains fragment refine function declaration.
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#ifndef FRAGMENT_REFINE_HPP
#define FRAGMENT_REFINE_HPP

//user includes
#include "bus_def.hpp"
#include "parsec/rabin.hpp"
//vitis hls includes
#define __gmp_const const
#include "ap_int.h"
#include "hls_stream.h"
#include "hls_math.h"

/**
 * @brief Takes big chunks and segments it into small chunks
 *
 * Uses the rabin fingerprint and convertes big chunk meta data to small chunk meta data
 *
 * @param meta_in big chunk meta
 * @param data_in data stream containing big chunk
 * @param end_in indicates end of process after each big chunk
 * @param meta_out small chunk meta
 * @param data_out data stream containing small chunk
 * @param end_out same as end_in for small chunk
 */
void fragment_refine(hls::stream< bc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool >  &end_in,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out);
#endif //FRAGMENT_REFINE_HPP
