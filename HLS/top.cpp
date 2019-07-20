/*
 * This file contains the top function with the interface for synthesis of the dedup toolchain
 */

#include "top.hpp"


//Round robin schedulers
template <typename M, typename D>
static void split(hls::stream< M > &meta_in,
		hls::stream< D > &data_in,
		hls::stream< bool > &end_in,
		int np,
		hls::stream< M > meta_out[],
		hls::stream< D > data_out[],
		hls::stream< bool > end_out[]){
	int n = 0;
	bool end = end_in.read();
	splitter: while(!end){
		if (!meta_in.empty()){
			end_out[n].write(false);

			M meta_data = meta_in.read();
			meta_out[n].write(meta_data);

			D buffer;
			for (int i = 0 ; i < hls::ceil((double)meta_data.size.to_long()*8 / buffer.length()) ; i++){
				data_out[n].write(data_in.read());
			}

		}
		n = ++n % np;

		end = end_in.read();
	}

	send_end_flag: for (int i = 0 ; i < np ; i++){
		end_out[i].write(true);
	}
}

template <typename M, typename D>
static void merge(hls::stream< M > meta_in[],
		hls::stream< D > data_in[],
		hls::stream< bool > end_in[],
		int np,
		hls::stream< M > &meta_out,
		hls::stream< D > &data_out,
		hls::stream< bool > &end_out){
	bool end = false;
	bool end_np[NP_MERGE];

	init_end_buffer: for (int i = 0 ; i < np ; i++){
		end_np[i] = false;
	}

	merger: while(!end){
		end = true;
		round_robin: for (int n = 0 ; n < np ; n++){
			if (!end_np[n]){
				end = false;

				if (!end_in[n].empty()){
					if (end_in[n].read()){
						end_np[n] = true;
						continue;
					}

					end_out.write(false);

					M meta_data = meta_in[n].read();
					meta_out.write(meta_data);

					D buffer; //dummy buffer for length info
					for (int i = 0 ; i < hls::ceil((double) meta_data.size.to_long()*8 / buffer.length()) ; i++){
						data_out.write(data_in[n].read());
					}
				}
			}
		}
	}
	end_out.write(true);
}


//Read and write functions
static void read_in(
		hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out){
	bool end = end_in.read();

	read_in_loop: while(!end){
		end_out.write(false);

		c_size_t size = size_in.read();
		size_out.write(size);

		read_file: for (c_size_t i = 0 ; i < MAX_FILE_SIZE ; i += 8 ){
			if (i >= size)
				break;

			out.write(in.read());
		}

		end = end_in.read();
	}

	end_out.write(true);
}


static void write_out(
		hls::stream< c_size_t > &size_in,
		hls::stream< ap_uint< 64 > > &in,
		hls::stream< bool > &end_in,
		hls::stream< c_size_t > &size_out,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< bool > &end_out){
	bool end = end_in.read();

	write_out_loop: while(!end){
		end_out.write(false);

		c_size_t size = size_in.read();
		size_out.write(size);

		write_file: for (c_size_t i = 0 ; i < MAX_FILE_SIZE ; i += 8 ){
			if (i >= size)
				break;

			out.write(in.read());
		}

		end = end_in.read();
	}

	end_out.write(true);
}





void top(hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out){
	//definitions
	//buffer
	hls::stream< ap_uint< 64 > , MAX_BIG_CHUNK_SIZE/64> 	in_buffer("in_buffer");
	hls::stream< c_size_t , 2> 								size_in_buffer("size_in_buffer");
	hls::stream< bool > 									end_in_buffer("end_in");

	hls::stream< ap_uint< 64 > , MAX_BIG_CHUNK_SIZE/64> 	out_buffer("out_buffer");
	hls::stream< c_size_t , 2> 								size_out_buffer("size_out_buffer");
	hls::stream< bool , 2 > 								end_out_buffer("end_out_buffer");

	hls::stream< bc_packet, 2 > 							post_fragment_meta_buffer("post_fragment_meta_buffer");
	hls::stream< c_data_t , SC_STREAM_SIZE> 				post_fragment_data_buffer("post_fragment_data_buffer");
	hls::stream< bool , 2 > 								post_fragment_end_buffer("post_fragment_end_buffer");

	hls::stream< sc_packet, 2 > 							pre_reorder_meta_buffer("pre_reorder_meta_buffer");
	hls::stream< c_data_t , SC_STREAM_SIZE> 				pre_reorder_data_buffer("pre_reorder_data_buffer");
	hls::stream< bool , 2 > 								pre_reorder_end_buffer("pre_reorder_end_buffer");

	hls::stream< sc_packet, 2 > 							post_refine_meta_buffer[NP];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_refine_data_buffer[NP];
	hls::stream< bool , 2 > 								post_refine_end_buffer[NP];

	hls::stream< sc_packet, 2 > 							post_dedup_meta_buffer[NP];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_dedup_data_buffer[NP];
	hls::stream< bool , 2 > 								post_dedup_end_buffer[NP];

	//splitter
	hls::stream< bc_packet , 2 > 							pre_refine_meta_split[NP][NP_REFINE];
	hls::stream< c_data_t , SC_STREAM_SIZE > 				pre_refine_data_split[NP][NP_REFINE];
	hls::stream< bool, 2 > 									pre_refine_end_split[NP][NP_REFINE];

	hls::stream< sc_packet , 2 > 							pre_dedup_meta_split[NP][NP_DEDUP];
	hls::stream< c_data_t , SC_STREAM_SIZE > 				pre_dedup_data_split[NP][NP_DEDUP];
	hls::stream< bool , 2 > 								pre_dedup_end_split[NP][NP_DEDUP];

	hls::stream< bc_packet , 2 > 							post_fragment_meta_split[NP];
	hls::stream< c_data_t , BC_STREAM_SIZE > 				post_fragment_data_split[NP];
	hls::stream< bool , 2 > 								post_fragment_end_split[NP];

	//merger
	hls::stream< sc_packet, 2 > 							post_refine_meta_merge[NP][NP_REFINE];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_refine_data_merge[NP][NP_REFINE];
	hls::stream< bool, 2 > 									post_refine_end_merge[NP][NP_REFINE];

	hls::stream< sc_packet, 2 > 							post_dedup_meta_merge[NP][NP_DEDUP];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_dedup_data_merge[NP][NP_DEDUP];
	hls::stream< bool , 2 > 								post_dedup_end_merge[NP][NP_DEDUP];

	hls::stream< sc_packet, 2 > 							pre_reorder_meta_merge[NP];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				pre_reorder_data_merge[NP];
	hls::stream< bool , 2 > 								pre_reorder_end_split[NP];

	//START OF PIPELINE
#pragma HLS DATAFLOW
	//read
	read_in(in, size_in, end_in, in_buffer, size_in_buffer, end_in_buffer);

	//segment into coarse grained chunks, serial
	fragment(in_buffer, size_in_buffer, end_in_buffer, post_fragment_meta_buffer, post_fragment_data_buffer, post_fragment_end_buffer);

	//split to parallel pipelines
	split< bc_packet , c_data_t >(post_fragment_meta_buffer,
			post_fragment_data_buffer, post_fragment_end_buffer, NP, post_fragment_meta_split, post_fragment_data_split, post_fragment_end_split);

	//NP parallel pipelines
	pipelines_parallel: for (int i = 0 ; i < NP ; i++){
#pragma HLS UNROLL

		//segment into fine grained chunks, NP_REFINE parallel
		split< bc_packet , c_data_t >(post_fragment_meta_split[i],
				post_fragment_data_split[i], post_fragment_end_split[i], NP_REFINE,
				pre_refine_meta_split[i], pre_refine_data_split[i], pre_refine_end_split[i]);
		refine_parallel: for (int n = 0; n < NP_REFINE ; n++){
#pragma HLS UNROLL
			fragment_refine(pre_refine_meta_split[i][n], pre_refine_data_split[i][n], pre_refine_end_split[i][n],
					post_refine_meta_merge[i][n], post_refine_data_merge[i][n], post_refine_end_merge[i][n]);
		}
		merge< sc_packet , c_data_t >(post_refine_meta_merge[i], post_refine_data_merge[i], post_refine_end_merge[i], NP_REFINE,
				post_refine_meta_buffer[i], post_refine_data_buffer[i], post_refine_end_buffer[i]);

		//deduplicate data stream
		split< sc_packet , c_data_t >(post_refine_meta_buffer[i], post_refine_data_buffer[i] , post_refine_end_buffer[i], NP_DEDUP,
				pre_dedup_meta_split[i], pre_dedup_data_split[i], pre_dedup_end_split[i]);
		dedup_parallel: for (int n = 0 ; n < NP_DEDUP ; n++){
#pragma HLS UNROLL
			dedup(pre_dedup_meta_split[i][n], pre_dedup_data_split[i][n], pre_dedup_end_split[i][n],
					post_dedup_meta_merge[i][n], post_dedup_data_merge[i][n], post_dedup_end_merge[i][n]);
		}
		merge< sc_packet , c_data_t > (post_dedup_meta_merge[i], post_dedup_data_merge[i], post_dedup_end_merge[i], NP_DEDUP,
				post_dedup_meta_buffer[i], post_dedup_data_buffer[i], post_dedup_end_buffer[i]);
	}

	//merge from parallel pipelines
	merge< sc_packet , c_data_t >(post_dedup_meta_buffer, post_dedup_data_buffer, post_dedup_end_buffer, NP,
			pre_reorder_meta_buffer, pre_reorder_data_buffer, pre_reorder_end_buffer);

	//reorder the sc_packets for output
	reorder(pre_reorder_meta_buffer, pre_reorder_data_buffer, pre_reorder_end_buffer, size_out_buffer, out_buffer, end_out_buffer);

	//write out
	write_out(size_out_buffer, out_buffer, end_out_buffer, size_out, out, end_out);
}
