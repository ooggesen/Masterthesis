/**
 * @file top.cpp
 *
 * @brief Contains the top function with the interface for synthesis of the dedup toolchain.
 *
 * @author Ole Oggesen
 * @bug Not tested for successive parsing of files.
 */

#include "top.hpp"


/**
 * @brief Round robin splitter
 *
 * @param np 	Number of outputs
 */
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
#pragma HLS LOOP_FLATTEN off
		end_out[n].write(false);

		M meta_data = meta_in.read();
		meta_out[n].write(meta_data);

		D buffer;
		for (int i = 0 ; i < hls::ceil((double)meta_data.size.to_long()*8 / buffer.length()) ; i++){
#pragma HLS PIPELINE II=6
			data_out[n].write(data_in.read());
		}
		n = ++n % np;

		end = end_in.read();
	}

	send_end_flag: for (int i = 0 ; i < np ; i++){
		end_out[i].write(true);
	}
}



/**
 * @brief Round robin merger.
 *
 * Not blocking of input does not contain data.
 *
 * @param np Number of inputs
 */
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

	init_end_buffer: for (int i = 0 ; i < NP_MERGE ; i++){
		end_np[i] = false;
	}

	merger: while(!end){
#pragma HLS LOOP_FLATTEN  off
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
#pragma HLS PIPELINE II=6
						data_out.write(data_in[n].read());
					}
				}
			}
		}
	}
	end_out.write(true);
}


/**
 * @brief read in stage, needed for dataflow circuit
 */
static void read_in(
		hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out){
	bool end = end_in.read();

	read_in_loop: while(!end){
#pragma HLS LOOP_FLATTEN off
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



/**
 * @brief write out stage, needed for dataflow circuit
 */
static void write_out(
		hls::stream< c_size_t > &size_in,
		hls::stream< ap_uint< 64 > > &in,
		hls::stream< bool > &end_in,
		hls::stream< c_size_t > &size_out,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< bool > &end_out){
	bool end = end_in.read();

	write_out_loop: while(!end){
#pragma HLS LOOP_FLATTEN off
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
	hls::stream< bool , 2> 									end_in_buffer("end_in");

	hls::stream< ap_uint< 64 > , MAX_BIG_CHUNK_SIZE/64> 	out_buffer("out_buffer");
	hls::stream< c_size_t , 2> 								size_out_buffer("size_out_buffer");
	hls::stream< bool , 2 > 								end_out_buffer("end_out_buffer");

	hls::stream< bc_packet, 2 > 							post_fragment_meta_buffer("post_fragment_meta_buffer");
	hls::stream< c_data_t , SC_STREAM_SIZE> 				post_fragment_data_buffer("post_fragment_data_buffer");
	hls::stream< bool , 2 > 								post_fragment_end_buffer("post_fragment_end_buffer");

	hls::stream< sc_packet, 2 > 							pre_reorder_meta_buffer("pre_reorder_meta_buffer");
	hls::stream< c_data_t , SC_STREAM_SIZE> 				pre_reorder_data_buffer("pre_reorder_data_buffer");
	hls::stream< bool , 2 > 								pre_reorder_end_buffer("pre_reorder_end_buffer");

	hls::stream< sc_packet, 2 > 							post_refine_meta_buffer("post_refine_meta_buffer");
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_refine_data_buffer("post_refine_data_buffer");
	hls::stream< bool , 2 > 								post_refine_end_buffer("post_refine_end_buffer");

	hls::stream< sc_packet, 2 > 							post_dedup_meta_buffer("post_dedup_meta_buffer");
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_dedup_data_buffer("post_dedup_data_buffer");
	hls::stream< bool , 2 > 								post_dedup_end_buffer("post_dedup_end_buffer");

	//splitter
	hls::stream< bc_packet , 2 > 							pre_refine_meta_split[NP_REFINE];
	hls::stream< c_data_t , SC_STREAM_SIZE > 				pre_refine_data_split[NP_REFINE];
	hls::stream< bool, 2 > 									pre_refine_end_split[NP_REFINE];

	//merger
	hls::stream< sc_packet, 2 > 							post_refine_meta_merge[NP_REFINE];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_refine_data_merge[NP_REFINE];
	hls::stream< bool, 2 > 									post_refine_end_merge[NP_REFINE];

	//START OF PIPELINE
#pragma HLS DATAFLOW
	//read
	read_in(in, size_in, end_in, in_buffer, size_in_buffer, end_in_buffer);

	//segment into coarse grained chunks, serial
	fragment(in_buffer, size_in_buffer, end_in_buffer, post_fragment_meta_buffer, post_fragment_data_buffer, post_fragment_end_buffer);

	//segment into fine grained chunks, NP_REFINE parallel
	split< bc_packet , c_data_t >(post_fragment_meta_buffer,
			post_fragment_data_buffer, post_fragment_end_buffer, NP_REFINE,
			pre_refine_meta_split, pre_refine_data_split, pre_refine_end_split);
	refine_parallel: for (int n = 0; n < NP_REFINE ; n++){
#pragma HLS UNROLL
		fragment_refine(pre_refine_meta_split[n], pre_refine_data_split[n], pre_refine_end_split[n],
				post_refine_meta_merge[n], post_refine_data_merge[n], post_refine_end_merge[n]);
	}
	merge< sc_packet , c_data_t >(post_refine_meta_merge, post_refine_data_merge, post_refine_end_merge, NP_REFINE,
			post_refine_meta_buffer, post_refine_data_buffer, post_refine_end_buffer);

	dedup(post_refine_meta_buffer, post_refine_data_buffer, post_refine_end_buffer,
			pre_reorder_meta_buffer, pre_reorder_data_buffer, pre_reorder_end_buffer);

	//reorder the sc_packets for output
	reorder(pre_reorder_meta_buffer, pre_reorder_data_buffer, pre_reorder_end_buffer, size_out_buffer, out_buffer, end_out_buffer);

	//write out
	write_out(size_out_buffer, out_buffer, end_out_buffer, size_out, out, end_out);
}
