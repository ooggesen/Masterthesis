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

//function declarations
void fragment_refine(hls::stream< bc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool >  &end_in,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out);
#endif //FRAGMENT_REFINE_HPP
