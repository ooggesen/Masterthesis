#ifndef BRAM_H
#define BRAM_H

#include "bus_def.h"

#define MAX_BRAM_SIZE 1024//(5 * BIG_CHUNK_SIZE / SMALL_CHUNK_SIZE) //number of small chunk packages the complete BRAM can hold

struct bram_data{
	sc_data_t data[SC_ARRAY_SIZE];
	addr_t hash;
};

void bram(bool wren, bool rden,
		bram_packet packet_w,
		bram_packet &packet_r);
#endif //BRAM_H
