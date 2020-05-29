#ifndef BRAM_H
#define BRAM_H

#include "bus_def.h"

#define NUM_BRAM 16 //number of brams the buffer is partitioned into
#define MAX_BRAM_SIZE 4096 //number of packages the complete BRAM can hold

struct bram_data{
	sc_data_t data[SC_ARRAY_SIZE];
	addr_t hash;
};

void bram(bool wren, bool rden,
		bram_packet packet_w,
		bram_packet &packet_r);
void initialize_buffer();
#endif //BRAM_H
