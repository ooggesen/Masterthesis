/*
 * This file contains the definitions of the bus interface of the dedup pipeline stages
 */

#ifndef BUS_DEF_H
#define BUS_DEF_H

#include "ap_int.h"

//chunk size definitions
#define SMALL_CHUNK_SIZE (512 * 8) //average small chunk size in bits
#define BIG_CHUNK_SIZE (1024 * SMALL_CHUNK_SIZE) //in PARSEC: (2*1024*1024*8)-> too big for FPGA; average big chunk size in bits
//bus width definitions
#define W_DATA_SMALL_CHUNK 512 //must be a multiple of 32 !!! ; width of small chunk bus line in bits
#define W_DATA_BIG_CHUNK 1024 //width of big chunk bus line in bits
#define W_ADDR 160 //width of SHA1 signature in bits
#define W_CHUNK_SIZE 64 //width of size integer in bits, like in PARSEC
#define W_L1_ORDER 16 //width of l1 position integer in bits -> 2^16 * avgerage big chunk size(2 MB) = 130 GB
#define W_L2_ORDER 16 //width of l2 position integer in bits -> 2^16 * average small chunk size(512 bytes) > average big chunk size(2MB)
//relational definitions
#define SC_ARRAY_SIZE ((int) 40 * SMALL_CHUNK_SIZE/W_DATA_SMALL_CHUNK) //size of data array containing a small chunk
#define BC_ARRAY_SIZE ((int) (BIG_CHUNK_SIZE+40*SMALL_CHUNK_SIZE)/W_DATA_BIG_CHUNK) //size of data array containing a big chunk
// type definitions
typedef ap_uint<W_DATA_SMALL_CHUNK> sc_data_t; //contains small chunks
typedef ap_uint<W_DATA_BIG_CHUNK> bc_data_t; //contains big chunks
typedef ap_uint<W_ADDR> addr_t; //adddr for BRAM access
typedef ap_uint<W_CHUNK_SIZE> c_size_t; //size of data chunk in bytes
typedef ap_uint<W_L1_ORDER> l1_pos_t; //level 1 position of chunk
typedef ap_uint<W_L2_ORDER> l2_pos_t; //level 2 position of chunk


//big chunk bus interface
struct bc_packet{
	bc_data_t data[BC_ARRAY_SIZE];
	c_size_t size;
	l1_pos_t l1_pos;

	bc_packet(const bc_packet &in);
	bc_packet();
};

//small chunk bus interface
struct sc_packet{
	sc_data_t data[SC_ARRAY_SIZE];
	addr_t hash;
	c_size_t size;
	l1_pos_t l1_pos;
	l2_pos_t l2_pos;
	bool last_l2_chunk;
	bool is_duplicate;

	sc_packet(const sc_packet &in);
	sc_packet();
};

//BRAM access interface
struct bram_packet{
	sc_data_t data[SC_ARRAY_SIZE];
	addr_t addr;
};


//helper functions
bool is_equal(const sc_data_t a[SC_ARRAY_SIZE], const sc_data_t b[SC_ARRAY_SIZE]);
bool operator==(const bc_packet &a, const bc_packet &b);
bool operator!=(const bc_packet &a, const bc_packet &b);
bool operator==(const sc_packet &a, const sc_packet &b);
bool operator!=(const sc_packet &a, const sc_packet &b);

#endif //BUS_DEF_H
