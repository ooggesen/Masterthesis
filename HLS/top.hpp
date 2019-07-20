#ifndef TOP_HPP
#define TOP_HPP

#define MAX(a,b)         ((a < b) ?  (b) : (a))
#define BIGGEST(a,b,c)   ((MAX(a,b) < c) ?  (c) : (MAX(a,b)))

//Number of Threads
#define NP 1
#define NP_REFINE 2
#define NP_DEDUP 2
#define NP_MERGE BIGGEST(NP, NP_REFINE, NP_DEDUP)
//vitis hls includes
#define __gmp_const const
#include "ap_int.h"
#include "hls_stream.h"
#include "hls_math.h"

//user includes
#include "bus_def.hpp"
#include "fragment.hpp"
#include "fragment_refine.hpp"
#include "parsec/rabin.hpp"
#include "dedup.hpp"
#include "reorder.hpp"
#include "reorder_buffer.hpp"

//function declarations
void top(hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out);


#endif //TOP_HPP
