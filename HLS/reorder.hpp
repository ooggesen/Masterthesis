#ifndef REORDER_H
#define REORDER_H

//user libraries
#include "bus_def.hpp"
#include "parsec.hpp"
#include "reorder_buffer.hpp"
//vitis libraries
#include "hls_stream.h"
#define __gmp_const const
#include "ap_int.h"
#include "hls_math.h"

//function declarations
void reorder(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool > &end_in,
		hls::stream< c_size_t > &size_out,
		hls::stream< ap_uint< 64 > > &data_out,
		hls::stream< bool > &end_out);

#endif //REORDER_H
