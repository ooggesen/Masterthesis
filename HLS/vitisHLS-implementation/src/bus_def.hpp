/*
 * @file bus_def.hpp
 *
 * @brief Contains macros and function definitions for the bus interface and file limitations
 *
 * This file contains all restrictions for the top module and it is used in every pipeline stage.
 * Limitations to the choice of macros and descriptions are given in comments in the same line.
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#ifndef BUS_DEF_HPP
#define BUS_DEF_HPP

//bug fix for rtl/c simulation
#define __gmp_const const //fix to a bug with ap_int.h in VitisHLS 2021.2
#include "ap_int.h"
#include "hls_math.h"

//file definitions
#define MAX_FILE_SIZE 2000000 //max file size in bytes
//chunk size definitions
#define SMALL_CHUNK_SIZE (512 * 8) //average small chunk size in bits, according to PARSEC paper
#define BIG_CHUNK_SIZE (64 * SMALL_CHUNK_SIZE) //in PARSEC: (2*1024*1024*8)-> too big for FPGA; average big chunk size in bits
//bus width definitions
#define W_DATA 1024 //must be a multiple of 64 for compliance with the sha1 kernel and input output stages
#define W_ADDR 160 //width of SHA1 signature in bits
#define W_CHUNK_SIZE 64 //width of size integer in bits, like in PARSEC
#define W_L1_ORDER 16 //width of l1 position integer in bits -> 2^16 * avgerage big chunk size(2 MB) = 130 GB > MAX_FILE_SIZE
#define W_L2_ORDER 16 //width of l2 position integer in bits -> 2^16 * average small chunk size(512 bytes) > average big chunk size(2MB)
//relational definitions
#define MAX_SMALL_CHUNK_SIZE  	(20 * SMALL_CHUNK_SIZE) //maximum size of a small chunk in bits
#define MAX_BIG_CHUNK_SIZE		(MAX_SMALL_CHUNK_SIZE + BIG_CHUNK_SIZE) //maximum size of a big chunk in bits
#define SC_STREAM_SIZE 			(int) (MAX_SMALL_CHUNK_SIZE/W_DATA + 1) //size of data array containing a small chunk
#define BC_STREAM_SIZE 			(int) (MAX_BIG_CHUNK_SIZE/W_DATA + 1) //size of data array containing a big chunk
// type definitions
typedef ap_uint<W_DATA> c_data_t; //contains small and big chunks chunks
typedef ap_uint<W_ADDR> addr_t; //address for BRAM access
typedef ap_uint<W_CHUNK_SIZE> c_size_t; //size of data chunk in bytes
typedef ap_uint<W_L1_ORDER> l1_pos_t; //level 1 position of chunk
typedef ap_uint<W_L2_ORDER> l2_pos_t; //level 2 position of chunk


/**
 * Meta data for coarse grained chunks (big chunks)
 */
struct bc_packet{
	c_size_t size;
	l1_pos_t l1_pos;

	bc_packet(const bc_packet &in);
	bc_packet();
};

/**
 * Meta data for fine grained chunks (small chunks)
 */
struct sc_packet{
	addr_t hash;
	c_size_t size;
	l1_pos_t l1_pos;
	l2_pos_t l2_pos;
	bool last_l2_chunk;
	bool is_duplicate;

	sc_packet(const sc_packet &in);
	sc_packet();
};

/**
 * BRAM data access interface
 */
struct bram_packet{
	c_data_t data[SC_STREAM_SIZE];
	addr_t addr;

	bram_packet();
};


/**
 * @brief Compares c_data_t type arrays
 *
 * @param a input array 1
 * @param b input array 2
 * @size how many bytes are compared
 *
 * @return true if a and b are equal, with respect to size.
 */
bool is_equal(const c_data_t a[SC_STREAM_SIZE], const c_data_t b[SC_STREAM_SIZE], c_size_t &size);



//operator definitions
bool operator==(const bc_packet &a, const bc_packet &b);
bool operator!=(const bc_packet &a, const bc_packet &b);
bool operator==(const sc_packet &a, const sc_packet &b);
bool operator!=(const sc_packet &a, const sc_packet &b);

#endif //BUS_DEF_HPP
