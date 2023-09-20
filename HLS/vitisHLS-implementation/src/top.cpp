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
	bool end = end_in.read();

	read_in_loop: while(!end){
#pragma HLS LOOP_TRIPCOUNT min=1 max=1 avg=1
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
#pragma HLS LOOP_TRIPCOUNT min=1 max=1 avg=1
#pragma HLS LOOP_FLATTEN off
		end_out.write(false);

		//pass all data until size information arrives
		c_size_t written;
		buffer_spill: for (written = 0; written < MAX_FILE_SIZE ; written += 8){
			while(in.empty() && size_in.empty());

			if (!size_in.empty())
				break;

			out.write(in.read());
		}


		//write the rest
		c_size_t size = size_in.read();
		size_out.write(size);
		write_rest_data: for ( ; written < MAX_FILE_SIZE ; written += 8){
			if (written >= size)
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
//#pragma HLS INTERFACE mode=ap_ctrl_chain port=return
	//definitions
	//buffer
	hls::stream< ap_uint< 64 > , W_DATA/64> 				in_buffer("in_buffer");
	hls::stream< c_size_t , 2 > 							size_in_buffer("size_in_buffer");
	hls::stream< bool , 2> 									end_in_buffer("end_in");

	hls::stream< ap_uint< 64 > , W_DATA/64 > 				out_buffer("out_buffer");
	hls::stream< c_size_t , 2 > 							size_out_buffer("size_out_buffer");
	hls::stream< bool , 2 > 								end_out_buffer("end_out_buffer");

	hls::stream< bc_packet, 2 > 							post_fragment_meta_buffer("post_fragment_meta_buffer");
	hls::stream< c_data_t , 4 > 							post_fragment_data_buffer("post_fragment_data_buffer");
	hls::stream< bool , 2 > 								post_fragment_end_buffer("post_fragment_end_buffer");

	hls::stream< sc_packet, 2 > 							pre_reorder_meta_buffer("pre_reorder_meta_buffer");
	hls::stream< c_data_t , SC_STREAM_SIZE > 				pre_reorder_data_buffer("pre_reorder_data_buffer");
	hls::stream< bool , 2 > 								pre_reorder_end_buffer("pre_reorder_end_buffer");

	hls::stream< sc_packet, 2 > 							post_refine_meta_buffer("post_refine_meta_buffer");
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_refine_data_buffer("post_refine_data_buffer");
	hls::stream< bool , 2 > 								post_refine_end_buffer("post_refine_end_buffer");

	//splitter
	hls::stream< bc_packet , 2 > 							pre_refine_meta_split[NP_REFINE];
	hls::stream< c_data_t , 8 > 							pre_refine_data_split[NP_REFINE];
	hls::stream< bool, 2 > 									pre_refine_end_split[NP_REFINE];

	//merger
	hls::stream< sc_packet, 2 > 							post_refine_meta_merge[NP_REFINE];
	hls::stream< c_data_t, SC_STREAM_SIZE > 				post_refine_data_merge[NP_REFINE];
	hls::stream< bool, 2 > 									post_refine_end_merge[NP_REFINE];

	//START OF PIPELINE
#pragma HLS DATAFLOW
	read_in(in, size_in, end_in, in_buffer, size_in_buffer, end_in_buffer);

	fragment(in_buffer, size_in_buffer, end_in_buffer, post_fragment_meta_buffer, post_fragment_data_buffer, post_fragment_end_buffer);

	split< bc_packet , c_data_t >(post_fragment_meta_buffer,
			post_fragment_data_buffer, post_fragment_end_buffer,
			pre_refine_meta_split, pre_refine_data_split, pre_refine_end_split);
	refine_parallel: for (int n = 0; n < NP_REFINE ; n++){
#pragma HLS UNROLL
		fragment_refine(pre_refine_meta_split[n], pre_refine_data_split[n], pre_refine_end_split[n],
				post_refine_meta_merge[n], post_refine_data_merge[n], post_refine_end_merge[n]);
	}
	merge< sc_packet , c_data_t >(post_refine_meta_merge, post_refine_data_merge, post_refine_end_merge,
			post_refine_meta_buffer, post_refine_data_buffer, post_refine_end_buffer);

	dedup(post_refine_meta_buffer, post_refine_data_buffer, post_refine_end_buffer,
			pre_reorder_meta_buffer, pre_reorder_data_buffer, pre_reorder_end_buffer);

	reorder(pre_reorder_meta_buffer, pre_reorder_data_buffer, pre_reorder_end_buffer, size_out_buffer, out_buffer, end_out_buffer);

	write_out(size_out_buffer, out_buffer, end_out_buffer, size_out, out, end_out);
}
