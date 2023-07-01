#ifndef FRAGMENT_REFINE_HPP
#define FRAGMENT_REFINE_HPP

//user includes
#include "bus_def.hpp"
#include "parsec/rabin.hpp"
//vitis hls includes
#include "hls_stream.h"
#include "hls_math.h"

//function declarations
void fragment_refine(hls::stream< bc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		bool end,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out);
#endif //FRAGMENT_REFINE_HPP
