/*
 * This file contains the top function with the interface for synthesis of the dedup toolchain
 */

#include "top.hpp"


//Round robin schedulers
template <typename T>
static void split(hls::stream< T > &in, bool end, int np, hls::stream< T > out[]){
	int n = 0;
	splitter: while(!in.empty() || !end){
		if (!in.empty())
			out[n++].write(in.read());
		n = n % np;
	}
}

template <typename T>
static void merge(hls::stream< T > in[], bool end, int np, hls::stream< T > &out){
	bool empty = false;
	merger: while(!end || !empty){
		empty = true;
		round_robin: for (int n = 0 ; n < np ; n++){
			if (!in[n].empty()){
				out.write(in[n].read());
				empty = false;
			}
		}
	}
}


//Read and write functions
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


//Updater for the end of process flags
static void update_fragment_end(bool &end, hls::stream< ap_uint< 8 > > &in, bool &fragment_end){
	fragment_end = end && in.empty();
}

static void update_refine_end(bool &fragment_end, hls::stream< ap_uint< 8 > > &in_buffer, bool &refine_end){
	refine_end = fragment_end && in_buffer.empty();
}

static void update_dedup_end(bool &refine_end, hls::stream< bc_packet > &post_fragment_split, bool &dedup_end){
	dedup_end = refine_end && post_fragment_split.empty();
}

static void update_reorder_end(bool dedup_end[NP], hls::stream< sc_packet , 2 > pre_dedup_split[NP][NP_DEDUP], bool &reorder_end){
	reorder_end = true;
	for (int i = 0 ; i < NP ; i++){
#pragma HLS UNROLL
		reorder_end = reorder_end && dedup_end[i];
		for (int j = 0 ; j < NP_DEDUP ; j++){
#pragma HLS UNROLL
			reorder_end = reorder_end && pre_dedup_split[i][j].empty();
		}
	}
}

static void update_write_out_end(bool &reorder_end, hls::stream< sc_packet > &pre_reorder_buffer, bool &write_out_end){
	write_out_end = reorder_end && pre_reorder_buffer.empty();
}



/*
 * Top function containing the dedup pipeline
 */
void top(hls::stream< ap_uint< 8 > > &in, bool end, hls::stream< ap_uint< 8 > > &out){
	//definitions
	//buffer
	hls::stream< ap_uint< 8 > , (BIG_CHUNK_SIZE + 40 * SMALL_CHUNK_SIZE)/8> in_buffer;
	hls::stream< ap_uint< 8 > , (BIG_CHUNK_SIZE + 40 * SMALL_CHUNK_SIZE)/8> out_buffer;//TODO check if depth of buffers suffices
	hls::stream< bc_packet, 2 > post_fragment_buffer;
	hls::stream< sc_packet, 2 > pre_reorder_buffer;
	hls::stream< sc_packet, 2 > post_refine_buffer[NP];
	hls::stream< sc_packet, 2 > post_dedup_buffer[NP];
	//splitter
	hls::stream< bc_packet , 2 > pre_refine_split[NP][NP_REFINE];
	hls::stream< sc_packet, 2 > pre_dedup_split[NP][NP_DEDUP];
	hls::stream< bc_packet, 2 > post_fragment_split[NP];
	//merger
	hls::stream< sc_packet, 2 > post_refine_merge[NP][NP_REFINE];
	hls::stream< sc_packet, 2 > post_dedup_merge[NP][NP_DEDUP];
	hls::stream< sc_packet, 2 > pre_reorder_merge[NP];
	//end of process flags
	bool fragment_end, refine_end, dedup_end[NP], reorder_end, write_out_end;

	//START OF PIPELINE
#pragma HLS DATAFLOW
	//read
	read_in(in, end, in_buffer);

	//segment into coarse grained chunks, serial
	update_fragment_end(end, in, fragment_end);
	fragment(in_buffer, fragment_end, post_fragment_buffer);

	//split to parallel pipelines
	split< bc_packet >(post_fragment_buffer, fragment_end, NP, post_fragment_split);

	//NP parallel pipelines
	pipelines_parallel: for (int i = 0 ; i < NP ; i++){
#pragma HLS UNROLL

		//segment into fine grained chunks, NP_REFINE parallel
		update_refine_end(fragment_end, in_buffer, refine_end);
		split< bc_packet >(post_fragment_split[i], refine_end, NP_REFINE, pre_refine_split[i]);
		refine_parallel: for (int n = 0; n < NP_REFINE ; n++){
#pragma HLS UNROLL
			fragment_refine(pre_refine_split[i][n], refine_end, post_refine_merge[i][n]);
		}
		merge< sc_packet >(post_refine_merge[i], refine_end, NP_REFINE, post_refine_buffer[i]);

		//deduplicate data stream
		update_dedup_end(refine_end, post_fragment_split[i], dedup_end[i]);
		split< sc_packet >(post_refine_buffer[i], dedup_end[i], NP_DEDUP, pre_dedup_split[i]);
		dedup_parallel: for (int n = 0 ; n < NP_DEDUP ; n++){
#pragma HLS UNROLL
			dedup(pre_dedup_split[i][n], dedup_end[i], post_dedup_merge[i][n]);
		}
		merge< sc_packet > (post_dedup_merge[i], dedup_end[i], NP_DEDUP, post_dedup_buffer[i]);
	}

	//merge from parallel pipelines
	update_reorder_end(dedup_end, pre_dedup_split, reorder_end);
	merge< sc_packet >(post_dedup_buffer, reorder_end, NP, pre_reorder_buffer);

	//reorder the sc_packets for output
	reorder(pre_reorder_buffer, reorder_end, out_buffer);

	//write out
	update_write_out_end(reorder_end, pre_reorder_buffer, write_out_end);
	write_out(out_buffer, write_out_end, out);
}
