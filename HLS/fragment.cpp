/*
 * This file contains the kernel for the coarse grained chunking algorithm
 */

#include "fragment.hpp"


#define IN_BUFFER_SIZE ((BIG_CHUNK_SIZE + 40 * SMALL_CHUNK_SIZE)/8) //Expected maximum size for a big chunk



static void fill_buffer(hls::stream< ap_uint< 8 > > &in, bool end, hls::stream< ap_uint< 8 > > &buffer){
	fill_buffer_loop: for (int i = 0 ;  i < (BIG_CHUNK_SIZE - 10*SMALL_CHUNK_SIZE)/8 ; i++){
		if (end && in.empty()) return;
		buffer.write(in.read());
	}
}

static void write_out(hls::stream< ap_uint< 8 > > &buffer, unsigned l1, bool end, hls::stream< bc_packet > &out){
	unsigned long long size = 0;

	//write data
	bc_packet bcp;
	stream_to_big_chunk: for (int arr_pos = 0 ; arr_pos < BC_ARRAY_SIZE ; arr_pos++){
		for (int byte_pos = 0 ; byte_pos < W_DATA_BIG_CHUNK/8 ; byte_pos++){
			if (!buffer.empty()){
				bcp.data[arr_pos].range(7 + 8 * byte_pos, 8 * byte_pos) = buffer.read(); //TODO does this fit with the write out? MSB first LSB first?
				size++;
			} else {
				bcp.data[arr_pos].range(7 + 8 * byte_pos, 8 * byte_pos) = 0; //fill with 0
			}
		}
	}

	//write metadata
	bcp.l1_pos = l1;
	bcp.size = size;
	bcp.end = end;

	//write to out stream
	out.write(bcp);
}

void fragment(hls::stream< ap_uint< 8 > > &in, bool end,  hls::stream< bc_packet > &out){
	//intialize the rabin lookup tables
	int winlen = 0;
	unsigned rabintab[256], rabinwintab[256];
	rabininit(winlen, rabintab, rabinwintab);

	//run segmentation until end
	unsigned  l1 = 0;
	hls::stream< ap_uint< 8 > , IN_BUFFER_SIZE > buffer; //contains big chunk data
	fragment_stream: while(!end || !in.empty()){
		//fill the buffer with a minimum of data for a big chunk packet
		fill_buffer(in, end, buffer);

		//segment the input data
		rabinseg_in_stream(in, end, buffer, winlen, rabintab, rabinwintab);

		//convert to big chunk packet
		write_out(buffer, l1, end && in.empty() , out);
		l1++;
	}
}
