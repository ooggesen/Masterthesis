#ifndef REORDER_H
#define REORDER_H

#include "bus_def.h"
#include "hls_stream.h"

#define STACK_DEPTH 10

typedef struct {
	unsigned sp;
	bus_packet stack[STACK_DEPTH];
} stack;

#endif //REORDER_H
