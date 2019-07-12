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
void fragment(hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< bc_packet > &meta_out,
		hls::stream< c_data_t > &out,
		hls::stream< bool > &end_out);

#endif //FRAGMENT_HPP
