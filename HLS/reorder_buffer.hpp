#ifndef REORDER_BUFFER_HPP
#define REORDER_BUFFER_HPP

#include "bus_def.hpp"
#include "top.hpp"

//definitions
#define BUFFER_SIZE_1 (NP * NP_REFINE * 2) //buffer depth for big chunks
#define BUFFER_SIZE_2 (NP * NP_REFINE * 2 + 100) //buffer depth for small chunk

struct buffer_cell {
	sc_packet data;
	bool valid;
};

//access functions
void write_buffer(sc_packet &to_write, buffer_cell buffer[][BUFFER_SIZE_2]);
void read_buffer(l1_pos_t &l1, l2_pos_t &l2, buffer_cell buffer[][BUFFER_SIZE_2], sc_packet &out, bool &success);


#endif //REORDER_BUFFER_HPP
