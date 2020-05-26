#ifndef BRAM_H
#define BRAM_H

#include "bus_def.h"

#define NUM_BRAM 16 //number of brams the buffer is partitioned into

void bram(bool wren, bool rden,
		bram_packet packet_w,
		bram_packet &packet_r);
#endif //BRAM_H
