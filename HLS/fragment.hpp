#ifndef FRAGMENT_HPP
#define FRAGMENT_HPP

//user includes
#include "bus_def.hpp"
#include "parsec/rabin.hpp"
//vitis hls includes
#include "hls_stream.h"
#define __gmp_const const
#include "ap_int.h"
#include "hls_math.h"

//function declarations
void fragment(hls::stream< ap_uint< 8 > > &in, bool end,  hls::stream< bc_packet > &meta, hls::stream< c_data_t > &data );

#endif //FRAGMENT_HPP
