#ifndef REORDER_H
#define REORDER_H

//user libraries
#include "bus_def.h"
#include "parsec.h"
//vitis libraries
#include "hls_stream.h"
#include "ap_int.h"

//definitions
#define STACK_DEPTH 10

typedef struct {
	unsigned sp;
	bus_packet stack[STACK_DEPTH];
} stack;

//function declarations
void reorder(hls::stream< bus_packet > &in, hls::stream< ap_uint< 8 > > &out);

#endif //REORDER_H
