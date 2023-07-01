#ifndef REORDER_H
#define REORDER_H

//user libraries
#include "bus_def.hpp"
#include "parsec.hpp"
#include "reorder_buffer.hpp"
//vitis libraries
#include "hls_stream.h"
#include "ap_int.h"
#include "hls_math.h"

//function declarations
void reorder(hls::stream< sc_packet > &meta_in, hls::stream< c_data_t > &data_in, bool end, hls::stream< ap_uint< 8 > > &out);

#endif //REORDER_H
