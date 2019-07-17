/*
 * This file contains the interface to the bram modules used for the global hash table of the dedup kernels
 */


#include "bram.hpp"


static void increment_addr(unsigned long int &addr){
	if (++addr == MAX_BRAM_SIZE)
		addr = 0;
}



/*
 * bram function implements a hash table with linear probing
 * be aware: These functions do not implement bit stuffing with 0, if size is not a multiple of W_DATA * BRAM_DEPTH
 * 	-> stuff the data correctly when passing it to the BRAM
 *
 * TODO data is only reset by transmission of bit code
 */
void bram(bool wren, bool rden,
		bram_packet packet_w,
		bram_packet &packet_r,
		c_size_t size){
//#pragma HLS ARRAY_PARTITION variable=packet_w.data type=complete

	//define the bram array
	static bram_data buffer[MAX_BRAM_SIZE];
#pragma HLS BIND_STORAGE variable=buffer type=ram_2p
#pragma HLS ARRAY_PARTITION variable=buffer type=block factor=256 dim=1

	// linear probing: looking for entry that contains data with correct hash value to read from
	unsigned long int addr;
	//initial address
	if (rden)
		addr = packet_r.addr % MAX_BRAM_SIZE;
	else if (wren)
		addr = packet_w.addr % MAX_BRAM_SIZE;

	check_size: for (int i = 0 ; i < (int) MAX_SMALL_CHUNK_SIZE/BRAM_DEPTH + 1 ; i++){
		if (i >= hls::ceil((double) size.to_long()*8 / W_DATA / BRAM_DEPTH))
			break;

		find_addr: while (buffer[addr].hash != 0 && buffer[addr].hash != packet_r.addr){
#pragma HLS PIPELINE II=2
			increment_addr(addr);
		}

		//transfer data data
		if (wren) buffer[addr].hash =  packet_w.addr;
		transfer_loop_bram: for (int j = 0 ; j < BRAM_DEPTH ; j++){
#pragma HLS PIPELINE II=1
			if (rden)
				packet_r.data[i*BRAM_DEPTH + j] = buffer[addr].data[j];
			if (wren)
				buffer[addr].data[j] = packet_w.data[i*BRAM_DEPTH + j];
		}
		increment_addr(addr);
	}
}
