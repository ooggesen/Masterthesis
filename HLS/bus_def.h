/*
 * This file contains the definitions of the bus interface of the dedup pipeline stages
 */

#ifndef BUS_DEF_H
#define BUS_DEF_H

#include "ap_int.h"

//chunk size definitions
#define SMALL_CHUNK_SIZE 512 * 8 //512 bytes is average small chunk size in the PARCEL benchmark suite
#define BIG_CHUNK_SIZE 2000000
//bus width definitions
#define W_DATA_SMALL_CHUNK 512 // TODO adapt width for whole chunk transfer; must be a multiple of 32 !!!
#define W_DATA_BIG_CHUNK 1024 //TODO adapt width for whole chunk transfer
#define W_ADDR 160 //width of SHA1 signature
#define W_CHUNK_SIZE 22 //2^22 = 4.19 MB
#define W_L1_ORDER 16 //2^16 * avgerage big chunk size(2 MB) = 130 GB
#define W_L2_ORDER 16 //2^16 * average small chunk size(512 bytes) > average big chunk size(2MB)
//relational definitions
#define SC_ARRAY_SIZE 2*SMALL_CHUNK_SIZE/W_DATA_SMALL_CHUNK //size of array containing a small chunk
// type definitions
typedef ap_uint<W_DATA_SMALL_CHUNK> sc_data_t; //contains small chunks
typedef ap_uint<W_DATA_BIG_CHUNK> bc_data_t; //contains big chunks
typedef ap_uint<W_ADDR> addr_t; //adddr for BRAM access
typedef ap_uint<W_CHUNK_SIZE> c_size_t; //size of data chunk in bytes
typedef ap_uint<W_L1_ORDER> l1_pos_t; //level 1 position of chunk
typedef ap_uint<W_L2_ORDER> l2_pos_t; //level 2 position of chunk


//big chunk bus interface
typedef struct{
	bc_data_t data;
	c_size_t size;
	l1_pos_t l1_pos;
}bc_packet;

//small chunk bus interface
typedef struct{
	sc_data_t data[SC_ARRAY_SIZE];
	c_size_t size;
	l1_pos_t l1_pos;
	l2_pos_t l2_pos;
	bool is_duplicate;
}bus_packet;

//BRAM access interface
typedef struct{
	sc_data_t data;
	addr_t addr;
}bram_packet;

#endif //BUS_DEF_H
