/*
 * This file contains the top function with the interface for synthesis of the dedup toolchain
 */

#include "top.hpp"


//Round robin schedulers
template <typename M, typename D>
static void split(hls::stream< M > &meta_in,
		hls::stream< D > &data_in,
		bool end,
		int np,
		hls::stream< M > meta_out[],
		hls::stream< D > data_out[]){
	int n = 0;
	splitter: while(!meta_in.empty() || !end){
		if (!meta_in.empty()){
			M meta_data = meta_in.read();
			meta_out[n].write(meta_data);

			D buffer;
			for (int i = 0 ; i < hls::ceil((double)meta_data.size.to_long()*8 / buffer.length()) ; i++){
				data_out[n].write(data_in.read());
			}

		}
		n = ++n % np;
	}
}

template <typename M, typename D>
static void merge(hls::stream< M > meta_in[],
		hls::stream< D > data_in[],
		bool end,
		int np,
		hls::stream< M > &meta_out,
		hls::stream< D > &data_out){
	bool empty = false;
	merger: while(!end || !empty){
		empty = true;
		round_robin: for (int n = 0 ; n < np ; n++){
			if (!meta_in[n].empty()){
				M meta_data = meta_in[n].read();
				meta_out.write(meta_data);

				D buffer;
				for (int i = 0 ; i < hls::ceil((double) meta_data.size.to_long()*8 / buffer.length()) ; i++){
					data_out.write(data_in[n].read());
				}
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

static void update_reorder_end(bool dedup_end[NP], hls::stream< sc_packet , 1 > pre_dedup_split[NP][NP_DEDUP], bool &reorder_end){
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
	hls::stream< ap_uint< 8 > , (BIG_CHUNK_SIZE+40*SMALL_CHUNK_SIZE)/8> in_buffer("in_buffer");
	hls::stream< ap_uint< 8 > , (BIG_CHUNK_SIZE+40*SMALL_CHUNK_SIZE)/8> out_buffer("out_buffer");//TODO check if depth of buffers suffices
	hls::stream< bc_packet, NP > 							post_fragment_meta_buffer("post_fragment_meta_buffer");
	hls::stream< c_data_t , NP * SC_STREAM_SIZE> 			post_fragment_data_buffer("post_fragment_data_buffer");
	hls::stream< sc_packet, NP > 							pre_reorder_meta_buffer("pre_reorder_meta_buffer");
	hls::stream< c_data_t , NP * SC_STREAM_SIZE> 			pre_reorder_data_buffer("pre_reorder_data_buffer");
	hls::stream< sc_packet, 1 > 							post_refine_meta_buffer[NP];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_refine_data_buffer[NP];
	hls::stream< sc_packet, 1 > 							post_dedup_meta_buffer[NP];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_dedup_data_buffer[NP];
	//splitter
	hls::stream< bc_packet , 1 > 							pre_refine_meta_split[NP][NP_REFINE];
	hls::stream< c_data_t, SMALL_CHUNK_SIZE*40/8> 			pre_refine_data_split[NP][NP_REFINE];
	hls::stream< sc_packet, 1 > 							pre_dedup_meta_split[NP][NP_DEDUP];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				pre_dedup_data_split[NP][NP_DEDUP];
	hls::stream< bc_packet, 1 > 							post_fragment_meta_split[NP];
	hls::stream< c_data_t , BIG_CHUNK_SIZE/8 > 				post_fragment_data_split[NP];
	//merger
	hls::stream< sc_packet, 1 > 							post_refine_meta_merge[NP][NP_REFINE];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_refine_data_merge[NP][NP_REFINE];
	hls::stream< sc_packet, 1 > 							post_dedup_meta_merge[NP][NP_DEDUP];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_dedup_data_merge[NP][NP_DEDUP];
	hls::stream< sc_packet, 1 > 							pre_reorder_meta_merge[NP];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				pre_reorder_data_merge[NP];
	//end of process flags
	bool fragment_end, refine_end, dedup_end[NP], reorder_end, write_out_end;

	//START OF PIPELINE
#pragma HLS DATAFLOW
	//read
	read_in(in, end, in_buffer);

	//segment into coarse grained chunks, serial
	update_fragment_end(end, in, fragment_end);
	fragment(in_buffer, fragment_end, post_fragment_meta_buffer, post_fragment_data_buffer);

	//split to parallel pipelines
	split< bc_packet , c_data_t >(post_fragment_meta_buffer,
			post_fragment_data_buffer, fragment_end, NP, post_fragment_meta_split, post_fragment_data_split);

	//NP parallel pipelines
	pipelines_parallel: for (int i = 0 ; i < NP ; i++){
#pragma HLS UNROLL

		//segment into fine grained chunks, NP_REFINE parallel
		update_refine_end(fragment_end, in_buffer, refine_end);
		split< bc_packet , c_data_t >(post_fragment_meta_split[i],
				post_fragment_data_split[i], refine_end, NP_REFINE, pre_refine_meta_split[i], pre_refine_data_split[i]);
		refine_parallel: for (int n = 0; n < NP_REFINE ; n++){
#pragma HLS UNROLL
			fragment_refine(pre_refine_meta_split[i][n], pre_refine_data_split[i][n], refine_end, post_refine_meta_merge[i][n], post_refine_data_merge[i][n]);
		}
		merge< sc_packet , c_data_t >(post_refine_meta_merge[i], post_refine_data_merge[i], refine_end, NP_REFINE, post_refine_meta_buffer[i], post_refine_data_buffer[i]);

		//deduplicate data stream
		update_dedup_end(refine_end, post_fragment_meta_split[i], dedup_end[i]);
		split< sc_packet , c_data_t >(post_refine_meta_buffer[i], post_refine_data_buffer[i] , dedup_end[i], NP_DEDUP, pre_dedup_meta_split[i], pre_dedup_data_split[i]);
		dedup_parallel: for (int n = 0 ; n < NP_DEDUP ; n++){
#pragma HLS UNROLL
			dedup(pre_dedup_meta_split[i][n], pre_dedup_data_split[i][n], dedup_end[i], post_dedup_meta_merge[i][n], post_dedup_data_merge[i][n]);
		}
		merge< sc_packet , c_data_t > (post_dedup_meta_merge[i], post_dedup_data_merge[i], dedup_end[i], NP_DEDUP, post_dedup_meta_buffer[i], post_dedup_data_buffer[i]);
	}

	//merge from parallel pipelines
	update_reorder_end(dedup_end, pre_dedup_meta_split, reorder_end);
	merge< sc_packet , c_data_t >(post_dedup_meta_buffer, post_dedup_data_buffer, reorder_end, NP, pre_reorder_meta_buffer, pre_reorder_data_buffer);

	//reorder the sc_packets for output
	reorder(pre_reorder_meta_buffer, pre_reorder_data_buffer, reorder_end, out_buffer);

	//write out
	update_write_out_end(reorder_end, pre_reorder_meta_buffer, write_out_end);
	write_out(out_buffer, write_out_end, out);
}
