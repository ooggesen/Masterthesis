/*
 * This file contains the top function with the interface for synthesis of the dedup toolchain
 */

#include "top.hpp"

#define NP 2
#define NP_REFINE 2
#define NP_DEDUP 2

template <typename T>
static void split(hls::stream< T > &in, bool end, hls::stream< T > out[]){
	int n = 0;
	splitter: while(!in.empty() || !end){
		out[n++].write(in.read());
		n = n % NP;
	}
}

template <typename T>
static void merge(hls::stream< T > in[], bool end, hls::stream< T > &out){
	bool empty = false;
	merger: while(!end || !empty){
		empty = true;
		for (int n = 0 ; n < NP ; n++){
			if (!in[n].empty()){
				out.write(in[n].read());
				empty = false;
			}
		}
	}
}

static void read_in(hls::stream< ap_uint< 8 > > &in, bool end, hls::stream< ap_uint< 8 > > &buffer){
	while(!in.empty() || !end){
		buffer.write(in.read());
	}
}

static void write_out(hls::stream< sc_packet > &in, bool end, hls::stream< sc_packet > &out){
	sc_packet packet;
	do{
		packet = in.read();
		out.write(packet);
	}while(!end || !in.empty());
}




void top(hls::stream< ap_uint< 8 > > &in, bool end, hls::stream< sc_packet > &out){//hls::stream< ap_uint< 8 > > &out){
	//definitions
	//buffer
	hls::stream< ap_uint< 8 > , (BIG_CHUNK_SIZE + 40 * SMALL_CHUNK_SIZE)/8> in_buffer;
	hls::stream< ap_uint< 8 > , (BIG_CHUNK_SIZE + 40 * SMALL_CHUNK_SIZE)/8> out_buffer;//TODO check if depth of buffers suffices
	hls::stream< bc_packet, 2 > post_fragment_buffer;
	hls::stream< sc_packet, NP_REFINE * 2 > post_refine_buffer;
	//splitter
	hls::stream< bc_packet , 1 > pre_refine_split[NP_REFINE];
	hls::stream< sc_packet, 2 > pre_dedup_split[NP_DEDUP];
	//merger
	hls::stream< sc_packet, 2 > post_refine_merge[NP_REFINE];
	hls::stream< sc_packet, 2 > post_dedup_merge[NP_DEDUP];

	//START OF PIPELINE
	//read
	read_in(in, end, in_buffer);

	//segment into coarse grained chunks, serial
	fragment(in_buffer, end && in.empty(), post_fragment_buffer);

	//segment into fine grained chunks, parallel
	split< bc_packet >(post_fragment_buffer, end && in_buffer.empty(), pre_refine_split);
	for (int n = 0; n < NP ; n++){
		fragment_refine(pre_refine_split[n], end && post_fragment_buffer.empty(), post_refine_merge[n]);
	}
	merge< sc_packet >(post_refine_merge, end && post_fragment_buffer.empty(), post_refine_buffer);

	//write out
	write_out(post_refine_buffer, end && post_fragment_buffer.empty(), out);
}
