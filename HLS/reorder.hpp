#ifndef REORDER_H
#define REORDER_H

//user libraries
#include "bus_def.hpp"
#include "parsec.hpp"
#include "reorder_buffer.hpp"
//vitis libraries
#include "hls_stream.h"
#include "ap_int.h"

//function declarations
void reorder(hls::stream< sc_packet > &in, hls::stream< ap_uint< 8 > > &out);

#endif //REORDER_H
