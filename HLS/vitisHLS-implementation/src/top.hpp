/**
 * @file top.hpp
 *
 * @brief Top level function declaration
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */


#ifndef TOP_HPP
#define TOP_HPP

//Number of Threads
#define NP_REFINE 1
//vitis hls includes
#define __gmp_const const
#include "ap_int.h"
#include "hls_stream.h"
#include "hls_math.h"

//user includes
#include "bus_def.hpp"
#include "fragment.hpp"
#include "fragment_refine.hpp"
#include "dedup.hpp"
#include "reorder.hpp"
#include "scheduler.hpp"



/**
 * @brief Top level interface for the dedup kernel
 *
 *
 * @param in 		Input data stream
 * @param size_in	Size of input file
 * @param end_in	Indicates end of process after each file
 * @param out		Deduplicated output stream
 * @param size_out	Size of deduplicated file
 * @param end_out 	Indicates end of process after each file
 */
void top(hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out);


#endif //TOP_HPP
