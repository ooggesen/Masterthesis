#ifndef FRAGMENT_REFINE_H
#define FRAGMENT_REFINE_H

//user includes
#include "bus_def.hpp"
#include "parsec/rabin.hpp"
//vitis hls includes
#include "hls_stream.h"

//function declarations
void fragment_refine(hls::stream< bc_packet > &in, hls::stream< sc_packet > &out);

#endif //FRAGMENT_REFINE_H
