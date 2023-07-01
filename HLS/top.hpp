#ifndef TOP_HPP
#define TOP_HPP

//Number of Threads
#define NP 1
#define NP_REFINE 2
#define NP_DEDUP 2

//vitis hls includes
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
void top(hls::stream< ap_uint< 8 > > &in, bool end, hls::stream< ap_uint< 8 > > &out);


#endif //TOP_HPP
