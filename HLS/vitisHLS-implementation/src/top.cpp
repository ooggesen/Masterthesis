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
 * @brief read in stage, needed for dataflow circuit
 */
static void read_in(
		hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out){
	static bool end, init = true;
	static c_size_t size;
	static c_size_t it;

	if (!end){
		if (init){
			if (end_in.empty())
				return;
			if(end_in.read()){
				end = true;
				end_out.write(true);
				return;
			}

			it = 0;
			size = size_in.read();

			end_out.write(false);
			size_out.write(size);
			init = false;
		}

		read_file: for ( ; it < MAX_FILE_SIZE ; it += 8 ){
			if (it >= size || out.full())
				break;

			out.write(in.read());
		}

		if (it >= size){
			init = true;
		}
	}
}



/**
 * @brief write out stage, needed for dataflow circuit
 */
static void write_out(
		hls::stream< c_size_t > &size_in,
		hls::stream< ap_uint< 64 > > &in,
		hls::stream< bool > &end_in,
		bool &end,
		hls::stream< c_size_t > &size_out,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< bool > &end_out){
	if(!end_in.empty()){
		end = end_in.read();
		end_out.write(end);
	}

	//transfer data
	for (c_size_t i = 0 ; i < MAX_SMALL_CHUNK_SIZE / 8 * 2; i += 64 / 8){
		if (in.empty())
			break;

		out.write(in.read());
	}

	if (!size_in.empty()){
		size_out.write(size_in.read());
	}
}





void top(hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out){
//#pragma HLS INTERFACE mode=ap_ctrl_chain port=return
	//definitions
	//buffer
	hls::stream< ap_uint< 64 > , MAX_BIG_CHUNK_SIZE/64
	* NP_REFINE > 												in_buffer("in_buffer");
	hls::stream< c_size_t , NP_REFINE > 						size_in_buffer("size_in_buffer");
	hls::stream< bool , NP_REFINE > 							end_in_buffer("end_in");

	hls::stream< ap_uint< 64 > , MAX_SMALL_CHUNK_SIZE/64
	* NP_REFINE> 												out_buffer("out_buffer");
	hls::stream< c_size_t , NP_REFINE > 						size_out_buffer("size_out_buffer");
	hls::stream< bool , NP_REFINE > 							end_out_buffer("end_out_buffer");

	hls::stream< bc_packet, NP_REFINE > 						post_fragment_meta_buffer("post_fragment_meta_buffer");
	hls::stream< c_data_t , BC_STREAM_SIZE * NP_REFINE > 		post_fragment_data_buffer("post_fragment_data_buffer");
	hls::stream< bool , NP_REFINE > 							post_fragment_end_buffer("post_fragment_end_buffer");

	hls::stream< sc_packet, NP_REFINE > 						pre_reorder_meta_buffer("pre_reorder_meta_buffer");
	hls::stream< c_data_t , SC_STREAM_SIZE * NP_REFINE> 		pre_reorder_data_buffer("pre_reorder_data_buffer");
	hls::stream< bool , NP_REFINE > 							pre_reorder_end_buffer("pre_reorder_end_buffer");

	hls::stream< sc_packet, NP_REFINE > 						post_refine_meta_buffer("post_refine_meta_buffer");
	hls::stream< c_data_t, SC_STREAM_SIZE * NP_REFINE> 			post_refine_data_buffer("post_refine_data_buffer");
	hls::stream< bool , NP_REFINE > 							post_refine_end_buffer("post_refine_end_buffer");

	//flags
	bool end = false;
	//START OF PIPELINE
	parse_file: for (int num_bc = 0 ; num_bc < (int) MAX_FILE_SIZE * 8 / BIG_CHUNK_SIZE + 1 ; num_bc++){
		if (end)
			break;

		read_in(in, size_in, end_in, in_buffer, size_in_buffer, end_in_buffer);

		fragment(in_buffer, size_in_buffer, end_in_buffer, post_fragment_meta_buffer, post_fragment_data_buffer, post_fragment_end_buffer);

		parse_bc: for (int num_sc= 0 ; num_sc < (int) MAX_BIG_CHUNK_SIZE / SMALL_CHUNK_SIZE + 1 ; num_sc++){
			if (end)
				break;

			fragment_refine(post_fragment_meta_buffer, post_fragment_data_buffer, post_fragment_end_buffer,
					post_refine_meta_buffer, post_refine_data_buffer, post_refine_end_buffer);

			//big chunk completely parsed
			if (post_refine_end_buffer.empty())
				break;

			dedup(post_refine_meta_buffer, post_refine_data_buffer, post_refine_end_buffer,
					pre_reorder_meta_buffer, pre_reorder_data_buffer, pre_reorder_end_buffer);

			reorder(pre_reorder_meta_buffer, pre_reorder_data_buffer, pre_reorder_end_buffer, size_out_buffer, out_buffer, end_out_buffer);

			write_out(size_out_buffer, out_buffer, end_out_buffer, end, size_out, out, end_out);
		}
	}
}
