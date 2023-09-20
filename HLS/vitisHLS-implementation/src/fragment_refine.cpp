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
	bool end = end_in.read();

	while(!end){
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 1 avg = 1
#pragma HLS LOOP_FLATTEN off
		sc_packet sc_meta = meta_in.read();
		meta_out.write(sc_meta);
		end_out.write(false);

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

		end = end_in.read();
	}

	end_out.write(true);
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
	unsigned rabintab[256], rabinwintab[256];
	rabininit(rabintab, rabinwintab);

	bool end = end_in.read();

	while(!end){
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 1 avg = 1
#pragma HLS LOOP_FLATTEN off
		bc_packet bc_meta = meta_in.read();
		c_size_t bc_length = bc_meta.size;

		l2_pos_t l2 = 0;
		segment_bc: while(bc_length > 0){
#pragma HLS LOOP_FLATTEN off
			end_out.write(false);

			c_size_t sc_size;
			rabinseg_in_stream(in, bc_length, out, sc_size, rabintab, rabinwintab);

			sc_packet sc_meta;
			sc_meta.l1_pos = bc_meta.l1_pos;
			sc_meta.l2_pos = l2++;
			sc_meta.last_l2_chunk = (bc_length == 0);
			sc_meta.size = sc_size;
			//deafult values. Get correctly set by dedup kernel.
			sc_meta.hash = 0;
			sc_meta.is_duplicate = false;

			meta_out.write(sc_meta);
		}

		end = end_in.read();
	}

	end_out.write(true);
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

	bool end = end_in.read();

	while(!end){
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 1 avg = 1
#pragma HLS LOOP_FLATTEN off
		bc_packet bc_meta = meta_in.read();
		meta_out.write(bc_meta);
		end_out.write(false);

		read_data: for (c_size_t i = 0 ; i < MAX_BIG_CHUNK_SIZE / 8; i++){
#pragma HLS LOOP_FLATTEN  off
			if (i*W_DATA/8 >= bc_meta.size.to_long())
				break;

			c_data_t current_data = in.read();
			convert_to_byte: for (int j = 0 ; j < W_DATA / 8 ; j++){
#pragma HLS PIPELINE II=1
				if (i*W_DATA/8 + j < bc_meta.size)
					out.write(current_data.range(7 + 8*j, 8*j));
			}
		}

		end = end_in.read();
	}

	end_out.write(true);
}



void fragment_refine(hls::stream< bc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool >  &end_in,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out){
#pragma HLS DATAFLOW

	hls::stream< bc_packet, 2 > segment_meta("segment_meta");
	hls::stream< ap_uint< 8 > , MAX_BIG_CHUNK_SIZE/8 > segment_data("segment_data");
	hls::stream< bool, 2 > segment_end("segment_end");

	hls::stream< sc_packet, 2 > write_meta("write_meta");
	hls::stream< ap_uint< 8 > , MAX_SMALL_CHUNK_SIZE/8*2 > write_data("write_data");
	hls::stream< bool , 2 > write_end("write_end");

	convert_to_byte_stream(meta_in, data_in, end_in, segment_meta, segment_data, segment_end);

	segment_sc_packet(segment_meta, segment_data, segment_end, write_meta, write_data, write_end);

	write_out(write_meta, write_data, write_end, meta_out, data_out, end_out);
}
