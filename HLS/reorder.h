#ifndef REORDER_H
#define REORDER_H

//user libraries
#include "bus_def.h"
#include "parsec.h"
#include "reorder_buffer.h"
//vitis libraries
#include "hls_stream.h"
#include "ap_int.h"

//function declarations
void reorder(hls::stream< sc_packet > &in, hls::stream< ap_uint< 8 > > &out);

#endif //REORDER_H
