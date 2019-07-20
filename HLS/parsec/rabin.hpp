/**
 * @file rabin.hpp
 *
 * @brief Function definitions and macros for the rabin fingerprint algoritm
 *
 * https://github.com/bamos/parsec-benchmark/tree/master
 * path:
 * 	pkgs/kernels/dedup/src/rabin.h
 *
 * @author Ole Oggesen
 * @bug no known bugs
 */

#ifndef RABIN_HPP
#define RABIN_HPP

//user includes
#include "../bus_def.hpp"
#include "../parsec.hpp"
//vitis hls includes
#define __gmp_const const
#include "ap_int.h"
#include "hls_stream.h"

/* Define USED macro */
#define USED(x) { ulong y __attribute__ ((unused)); y = (ulong)x; }

enum {
  NWINDOW   = 32,
  MinSegment  = 1024,
  RabinMask = 0xfff,  // must be less than <= 0x7fff 
};


/**
 * @brief Initializes the rabintab and rabinwintab arrays used in te rabinseg_in_stream function
 */
void rabininit(unsigned rabintab[], unsigned rabinwintab[]);



/**
 * @brief Segments input stream according to the rabin fingerprint
 *
 * Needs access to the initialized rabintab and rabinwintab arrays.
 *
 * @param in 		Byte stream containing big chunk or file data
 * @param bc_size 	Bytes to read until end of file or end of big chunk
 * @param out 		Byte stream, segmented
 * @size 			Number of bytes until end of segment
 */
void rabinseg_in_stream(hls::stream< ap_uint< 8 > > &in,
		c_size_t &bc_size,
		hls::stream< ap_uint< 8 > > &out,
		c_size_t &size,
		unsigned rabintab[],
		unsigned rabinwintab[]);

#endif //RABIN_HPP

