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

//function declarations
void rabininit(unsigned rabintab[], unsigned rabinwintab[]);
void rabinseg_in_stream(hls::stream< ap_uint< 8 > > &in,
		c_size_t &bc_size,
		hls::stream< ap_uint< 8 > > &out,
		c_size_t &size,
		unsigned rabintab[],
		unsigned rabinwintab[]);

#endif //RABIN_HPP

