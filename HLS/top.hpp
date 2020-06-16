#ifndef TOP_HPP
#define TOP_HPP

//vitis hls includes
#include "ap_int.h"
#include "hls_stream.h"

//user includes
#include "bus_def.hpp"
#include "fragment.hpp"
#include "fragment_refine.hpp"
#include "rabin.hpp"
#include "dedup.hpp"
#include "reorder.hpp"
#include "reorder_buffer.hpp"

//function declarations
void top(hls::stream< ap_uint< 8 > > &in, bool end, hls::stream< sc_packet > &out); //intermediate top function-> until fragment refine or dedup


#endif //TOP_HPP
