/*
 * This file contains the top function with the interface for synthesis of the dedup toolchain
 */

#include "top.hpp"



template <typename T>
static void split(hls::stream< T > &in, bool end, int np, hls::stream< T > out[]){
	int n = 0;
	splitter: while(!in.empty() || !end){
		out[n++].write(in.read());
		n = n % np;
	}
}

template <typename T>
static void merge(hls::stream< T > in[], bool end, int np, hls::stream< T > &out){
	bool empty = false;
	merger: while(!end || !empty){
		empty = true;
		for (int n = 0 ; n < np ; n++){
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

static void write_out(hls::stream< ap_uint< 8 > > &in, bool end, hls::stream< ap_uint< 8 > > &out){
	do{
		out.write(in.read());
	}while(!end || !in.empty());
}




void top(hls::stream< ap_uint< 8 > > &in, bool end, hls::stream< ap_uint< 8 > > &out){
#pragma HLS DATAFLOW
	//definitions
	//buffer
	hls::stream< ap_uint< 8 > , (BIG_CHUNK_SIZE + 40 * SMALL_CHUNK_SIZE)/8> in_buffer;
	hls::stream< ap_uint< 8 > , (BIG_CHUNK_SIZE + 40 * SMALL_CHUNK_SIZE)/8> out_buffer;//TODO check if depth of buffers suffices
	hls::stream< bc_packet, 2 > post_fragment_buffer;
	hls::stream< sc_packet, 2 > pre_reorder_buffer;
	hls::stream< sc_packet, 2 > post_refine_buffer[NP];
	//splitter
	hls::stream< bc_packet , 2 > pre_refine_split[NP][NP_REFINE];
	hls::stream< sc_packet, 2 > pre_dedup_split[NP][NP_DEDUP];
	hls::stream< bc_packet, 2 > post_fragment_split[NP];
	//merger
	hls::stream< sc_packet, 2 > post_refine_merge[NP][NP_REFINE];
	hls::stream< sc_packet, 2 > post_dedup_merge[NP][NP_DEDUP];
	hls::stream< sc_packet, 2 > pre_reorder_merge[NP];

	//START OF PIPELINE
	//read
	read_in(in, end, in_buffer);

	//segment into coarse grained chunks, serial
	bool fragment_end = end && in.empty();
	fragment(in_buffer, fragment_end, post_fragment_buffer);

	//split to parallel pipelines
	split< bc_packet > (post_fragment_buffer, fragment_end, NP, post_fragment_split);

	//NP parallel pipelines
	bool refine_end, dedup_end;
	for (int i = 0 ; i < NP ; i++){
#pragma HLS UNROLL

		//segment into fine grained chunks, NP_REFINE parallel
		refine_end = fragment_end && in_buffer.empty();
		split< bc_packet >(post_fragment_split[i], refine_end, NP_REFINE, pre_refine_split[i]);
		for (int n = 0; n < NP_REFINE ; n++){
#pragma HLS UNROLL
			fragment_refine(pre_refine_split[i][n], refine_end, post_refine_merge[i][n]);
		}
		merge< sc_packet >(post_refine_merge[i], refine_end, NP_REFINE, post_refine_buffer[i]);
	}

	//merge from parallel pipelines
	merge< sc_packet >(post_refine_buffer, refine_end, NP, pre_reorder_buffer);

	//reorder the sc_packets for output
	bool reorder_end = refine_end && post_fragment_buffer.empty();
	reorder(pre_reorder_buffer, reorder_end, out_buffer);

	//write out
	bool write_out_end = reorder_end && pre_reorder_buffer.empty();
	write_out(out_buffer, write_out_end, out);
}
