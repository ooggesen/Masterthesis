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
		hls::stream< c_data_t > &data){
	//write data
	c_data_t_to_stream: for (int i = 0 ; i < BC_STREAM_SIZE ; i++){
#pragma HLS LOOP_FLATTEN off

		if (i >= hls::ceil((double) size.to_long()*8 / W_DATA))
			break;

		c_data_t c_buffer;
		byte_to_c_data_t: for (int j = 0 ; j < W_DATA/8 ; j++){
#pragma HLS LOOP_FLATTEN off
			if (size.to_long() > i*W_DATA/8 + j)
				c_buffer.range(7 + 8*j, 8*j) = buffer.read();
			else
				c_buffer.range(7 + 8*j, 8*j) = 0;
		}
		data.write(c_buffer);
	}

	//write metadata
	bc_packet bcp;
	bcp.l1_pos = l1++;
	bcp.size = size;

	//write to out stream
	meta.write(bcp);
}

static void segment_bc_packet(hls::stream< ap_uint< 8 > > &in,
		bool end,
		hls::stream< ap_uint< 8 > > &buffer,
		c_size_t &size,
		unsigned rabintab[],
		unsigned rabinwintab[]){
#pragma HLS PIPELINE off

	c_size_t bc_size;
	fill_buffer(in, end, bc_size, buffer);

	//segment the input data
	c_size_t sc_size;
	rabinseg_in_stream(in, end, buffer, sc_size, rabintab, rabinwintab);

	size = bc_size + sc_size;
}

void fragment(hls::stream< ap_uint< 8 > > &in, bool end,  hls::stream< bc_packet > &meta, hls::stream< c_data_t > &data ){
	hls::stream< ap_uint< 8 > , BC_STREAM_SIZE * W_DATA/8 > buffer("buffer"); //contains big chunk data
	//intialize the rabin lookup tables
	unsigned rabintab[256], rabinwintab[256];
	rabininit(rabintab, rabinwintab);

	//run segmentation until end
	unsigned  l1 = 0;
	fragment_stream: while(!end || !in.empty()){
#pragma HLS LOOP_FLATTEN off
		//fill the buffer with a minimum of data for a big chunk packet
		c_size_t size;
		segment_bc_packet(in, end, buffer, size, rabintab, rabinwintab);

		//convert to big chunk packet
		write_out(buffer, l1, size, meta, data);
	}
}
