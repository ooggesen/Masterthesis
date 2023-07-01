/*
 * This file contains the kernel for the coarse grained chunking algorithm
 */

#include "fragment.hpp"



#define MIN_BC_SIZE (BIG_CHUNK_SIZE - 10*SMALL_CHUNK_SIZE)



static void fill_buffer(hls::stream< ap_uint< 8 > > &in,
		bool end,
		c_size_t &read,
		hls::stream< ap_uint< 8 > > &buffer){
	c_size_t read_counter = 0;
	fill_buffer_loop: for (int i = 0 ;  i < MIN_BC_SIZE/8 ; i++){
		if (end && in.empty()){
			break;
		}
		buffer.write(in.read());
		read_counter++;
	}
	read = read_counter;
}

static void write_out(hls::stream< ap_uint< 8 > > &buffer,
		unsigned &l1,
		c_size_t size,
		hls::stream< bc_packet > &meta,
		hls::stream< ap_uint< 8 > > &data){
	//write data
	stream_to_big_chunk: for (c_size_t i = 0 ; i < size ; i++){
		data.write(buffer.read());
	}

	//write metadata
	bc_packet bcp;
	bcp.l1_pos = l1++;
	bcp.size = size;

	//write to out stream
	meta.write(bcp);
}

void fragment(hls::stream< ap_uint< 8 > > &in, bool end,  hls::stream< bc_packet > &meta, hls::stream< ap_uint< 8 > > &data ){
	//intialize the rabin lookup tables
	unsigned rabintab[256], rabinwintab[256];
	rabininit(rabintab, rabinwintab);

	//run segmentation until end
	unsigned  l1 = 0;
	hls::stream< ap_uint< 8 > , BC_STREAM_SIZE > buffer("buffer"); //contains big chunk data
	fragment_stream: while(!end || !in.empty()){
#pragma HLS PIPELINE
		//fill the buffer with a minimum of data for a big chunk packet
		c_size_t bc_size;
		fill_buffer(in, end, bc_size, buffer);

		//segment the input data
		c_size_t sc_size;
		rabinseg_in_stream(in, end, buffer, sc_size, rabintab, rabinwintab);

		//convert to big chunk packet
		write_out(buffer, l1, sc_size+bc_size, meta, data);
	}
}
