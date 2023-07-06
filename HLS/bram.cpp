/*
 * This file contains the interface to the bram modules used for the global hash table of the dedup kernels
 */


#include "bram.hpp"

/*
 * bram function implements a hash table with linear probing
 * be aware: These functions do not implement bit stuffing with 0, if size is not a multiple of BRAM_DEPTH
 * 	-> stuff the data correctly when passing it to the BRAM
 *
 * TODO data is only reset by transmission of bit code
 */
void bram(bool wren, bool rden,
		bram_packet packet_w,
		bram_packet &packet_r,
		c_size_t size){
#pragma HLS ARRAY_PARTITION variable=packet_w.data type=complete

	//define the bram array
	static bram_data buffer[MAX_BRAM_SIZE];
#pragma HLS BIND_STORAGE variable=buffer type=ram_2p
#pragma HLS ARRAY_PARTITION variable=buffer type=cyclic factor=512 dim=1

	if (rden){
	// linear probing: looking for entry that contains data with correct hash value to read from
	  unsigned long int addr = packet_r.addr % MAX_BRAM_SIZE;
	  read_check_size: for (int i = 0 ; i < hls::ceil((double) size.to_long()*8 / W_DATA / BRAM_DEPTH) ; i++){
		  find_addr_for_read: while (buffer[addr].hash != 0 && buffer[addr].hash != packet_r.addr)
			  ++addr;

		  //read data
		  read_loop_bram: for (int j = 0 ; j < BRAM_DEPTH ; j++){
			  packet_r.data[i*BRAM_DEPTH + j] = buffer[addr].data[j];
		  }
		  addr++;
	  }
	}

	if (wren){
	//linear probing: looking for next free entry after addr to store data into
	  unsigned long int addr = packet_w.addr % MAX_BRAM_SIZE;
	  write_check_size: for (int i = 0 ; i < hls::ceil((double) size.to_long()*8 / W_DATA / BRAM_DEPTH) ; i++){
		  find_addr_for_write: while (buffer[addr].hash != 0)
				  ++addr;

		  //write data
		  buffer[addr].hash =  packet_w.addr;
		  write_loop_bram: for (int j = 0 ; j < BRAM_DEPTH ; j++ ){
			  buffer[addr].data[j] = packet_w.data[i*BRAM_DEPTH + j];
		  }
		  addr++;
		}
	}
}
