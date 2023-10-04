/*
 * @file fragment_refine.cpp
 *
 * @brief Contains the fragment refine function and helper functions.
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "fragment_refine.hpp"


/**
 * @brief Converts byte stream to bus data stream and writes to output of fragment refine pipeline stage
 */
static void write_out(
		hls::stream< sc_packet > &meta_in,
		hls::stream< ap_uint< 8 > > &in,
		hls::stream< bool > &end_in,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &out,
		hls::stream< bool > &end_out){
	if (end_in.empty())
		return;

	if (end_in.read()){
		end_out.write(true);
		return;
	}

	end_out.write(false);

	sc_packet sc_meta = meta_in.read();
	meta_out.write(sc_meta);

	write_chunk: for (int i = 0 ; i < SC_STREAM_SIZE ; i++){
#pragma HLS LOOP_FLATTEN off
		if (i >= hls::ceil((double) sc_meta.size.to_long()*8 / W_DATA))
			break;

		c_data_t buffer;
		convert_to_c_data_t: for (c_size_t j = 0 ; j < W_DATA/8 ; j++){
#pragma HLS PIPELINE II=1
			if (sc_meta.size.to_long() > i*W_DATA/8 + j)
				buffer.range(7 + 8*j , 8*j) = in.read();
			else
				buffer.range(7 + 8*j , 8*j) = 0;
		}
		out.write(buffer);
	}
}


/**
 * @brief Segments the byte stream according to the rabin fingerprint.
 *
 * Generates small chunk meta information.
 */
static void segment_sc_packet(
		hls::stream< bc_packet > &meta_in,
		hls::stream< ap_uint< 8 > > &in,
		hls::stream< bool > &end_in,
		hls::stream< sc_packet > &meta_out,
		hls::stream< ap_uint< 8 > > &out,
		hls::stream< bool > &end_out){
	//intialize the rabin lookup tables
	static unsigned rabintab[256], rabinwintab[256];

	static bool end_seg, init_seg = true;
#pragma HLS RESET variable= end_seg
#pragma HLS RESET variable= init_seg
	static c_size_t bc_size =0;
#pragma HLS RESET variable= bc_size
	static l2_pos_t l2 = 0;
#pragma HLS RESET variable= l2
	static bc_packet meta_seg;
#pragma HLS RESET variable= meta_seg

	if(!end_seg){
		if (init_seg){
			if (end_in.empty())
				return;

			if (end_in.read()){
				end_out.write(true);
				end_seg = true;
				return;
			}

			rabininit(rabintab, rabinwintab);

			meta_seg = meta_in.read();
			bc_size = meta_seg.size;
			init_seg = false;
			l2 = 0;
		}


		if (bc_size > 0){
			end_out.write(false);

			c_size_t sc_size;
			rabinseg_in_stream(in, bc_size, out, sc_size, rabintab, rabinwintab);

			sc_packet sc_meta;
			sc_meta.l1_pos = meta_seg.l1_pos;
			sc_meta.l2_pos = l2++;
			sc_meta.last_l2_chunk = (bc_size == 0);
			sc_meta.size = sc_size;
			//default values. Get correctly set by dedup kernel.
			sc_meta.hash = 0;
			sc_meta.is_duplicate = false;

			meta_out.write(sc_meta);
		}

		if (bc_size == 0){
			init_seg = true;
		}
	}
}


/**
 * @brief Converts bus interface data to a byte stream.
 */
static void convert_to_byte_stream(
		hls::stream< bc_packet > &meta_in,
		hls::stream< c_data_t > &in,
		hls::stream< bool > & end_in,
		hls::stream< bc_packet > &meta_out,
		hls::stream< ap_uint< 8 > > &out,
		hls::stream< bool > &end_out){

	static bool end_byte, init_byte = true;
#pragma HLS RESET variable= end_byte
#pragma HLS RESET variable= init_byte
	static bc_packet meta_byte;
#pragma HLS RESET variable= meta_byte
	static c_size_t input_counter_byte;
#pragma HLS RESET variable= input_counter_byte
	static c_data_t current_long;
#pragma HLS RESET variable=current_long
	static int pos_byte = 0;
#pragma HLS RESET variable= pos_byte

	if (!end_byte){
		if (init_byte){
			if (end_in.empty())
				return;

			if (end_in.read()){
				end_byte = true;
				end_out.write(true);
				return;
			}

			meta_byte = meta_in.read();
			input_counter_byte = 0;

			end_out.write(false);
			meta_out.write(meta_byte);
			init_byte = false;
			pos_byte = 0;
		}

		read_data: for (c_size_t i = 0 ; i < SC_STREAM_SIZE + 1; i++){
#pragma HLS LOOP_FLATTEN off
			if (input_counter_byte + pos_byte >= meta_byte.size)
				break;

			if (pos_byte == 0)
				current_long = in.read();

			convert_to_byte: for (int j = 0 ; j < W_DATA / 8 ; j++){
				if (input_counter_byte + pos_byte < meta_byte.size && pos_byte < W_DATA/8){
					if (out.write_nb(current_long.range(7 + 8*pos_byte, 8*pos_byte)))
						pos_byte++;
					else
						return;
				}
			}

			if (pos_byte == W_DATA/8){
				input_counter_byte += W_DATA / 8;
				pos_byte = 0;
			}
		}

		if (input_counter_byte + pos_byte >= meta_byte.size){
			init_byte = true;
		}
	}
}



void fragment_refine(hls::stream< bc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool >  &end_in,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out){
	static hls::stream< bc_packet, 2 > 								segment_meta("segment_meta");
	static hls::stream< ap_uint< 8 > , MAX_BIG_CHUNK_SIZE/8 > 	segment_data("segment_data"); //stream size must be a multiple of W_DATA/8 to avoid deadlocks
	static hls::stream< bool, 2 > 									segment_end("segment_end");

	hls::stream< sc_packet, 2 > 									write_meta("write_meta");
	hls::stream< ap_uint< 8 > , MAX_SMALL_CHUNK_SIZE/8 >			write_data("write_data");
	static hls::stream< bool , 2 > 									write_end("write_end");

	convert_to_byte_stream(meta_in, data_in, end_in, segment_meta, segment_data, segment_end);

	segment_sc_packet(segment_meta, segment_data, segment_end, write_meta, write_data, write_end);

	write_out(write_meta, write_data, write_end, meta_out, data_out, end_out);
}
