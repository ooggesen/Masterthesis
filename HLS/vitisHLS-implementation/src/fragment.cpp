/*
 * @file fragment.cpp
 *
 * @brief Contains the definition of the pipeline stage for coarse grained chunking algorithm.
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "fragment.hpp"


//defines how many bytes are put into big chunk before rabin fingerprint
#define MIN_BC_SIZE (BIG_CHUNK_SIZE - 10*SMALL_CHUNK_SIZE)



/**
 * @brief Writes big chunk data to the output of the fragment pipeline stage
 *
 * Converts byte stream to c_data_t type and generates the meta data for the big chunk.
 */
static void write_out(
		hls::stream< ap_uint< 8 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< bc_packet > &meta_out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out){

	bool end = end_in.read();
	l1_pos_t l1 = 0;

	while(!end){
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 1 avg = 1
#pragma HLS LOOP_FLATTEN off
		c_size_t chunk_length = size_in.read();

		//write meta data
		bc_packet tmp_meta;
		tmp_meta.l1_pos = l1++;
		tmp_meta.size = chunk_length;
		meta_out.write(tmp_meta);

		end_out.write(false);

		//write data
		write_chunk: for (int i = 0 ; i < (int) MAX_BIG_CHUNK_SIZE / W_DATA + 1 ; i++){
#pragma HLS LOOP_FLATTEN off
			if (i >= hls::ceil((double) chunk_length.to_long()*8 / W_DATA))
				break;

			c_data_t buffer;
			convert_to_c_data_t: for (c_size_t j = 0 ; j < W_DATA/8 ; j++){
#pragma HLS PIPELINE II=1
				if (chunk_length.to_long() > i*W_DATA/8 + j)
					buffer.range(7 + 8*j , 8*j) = in.read();
				else
					buffer.range(7 + 8*j , 8*j) = 0;
			}
			data_out.write(buffer);
		}

		end = end_in.read();
	}

	end_out.write(true);
}



/**
 * @brief fills the buffer with a minimum number of bytes defined by the MIN_BC_SIZE macro
 */
static void fill_buffer(hls::stream< ap_uint< 8 > > &in,
		c_size_t &file_length,
		c_size_t &read,
		hls::stream< ap_uint< 8 > > &out){
	c_size_t file_length_buffer = file_length;
	c_size_t read_counter = 0;
	fill_buffer_loop: for (int i = 0 ;  i < MIN_BC_SIZE/8 ; i++){
		if (file_length_buffer == 0){
			break;
		}
		out.write(in.read());
		read_counter++;
		file_length_buffer--;
	}

	file_length = file_length_buffer;
	read = read_counter;
}



static void segment_bc_packet(
		hls::stream< ap_uint< 8 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > & end_in,
		hls::stream< ap_uint< 8 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out){
	//intialize the rabin lookup tables
	unsigned rabintab[256], rabinwintab[256];
#pragma HLS BIND_STORAGE variable=rabintab type=rom_1p
#pragma HLS BIND_STORAGE variable=rabinwintab type=rom_1p
	rabininit(rabintab, rabinwintab);

	bool end = end_in.read();

	while(!end){
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 1 avg = 1
#pragma HLS PIPELINE off
		c_size_t file_length = size_in.read();

		segment_file: while(file_length > 0){
#pragma HLS LOOP_FLATTEN off
			end_out.write(false);

			c_size_t bc_size;
			fill_buffer(in, file_length, bc_size, out);

			c_size_t sc_size;
			rabinseg_in_stream(in, file_length, out, sc_size, rabintab, rabinwintab);

			size_out.write(sc_size + bc_size);
		}

		end = end_in.read();
	}

	end_out.write(true);
}



static void convert_to_byte_stream(
		hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< ap_uint< 8 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out){

	bool end = end_in.read();

	while(!end){
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 1 avg = 1
		c_size_t file_length = size_in.read();
		end_out.write(false);
		size_out.write(file_length);

		read_data: for (c_size_t i = 0 ; i < (int) MAX_FILE_SIZE/8 ; i++){
#pragma HLS PIPELINE II=8
			if (i*8 >= file_length)
				break;

			ap_uint< 64 > current_long = in.read();
			convert_to_byte: for (int j = 0 ; j < 8 ; j++){
				if (i*8 + j < file_length)
					out.write(current_long.range(7 + 8*j, 8*j));
			}
		}

		end = end_in.read();
	}

	end_out.write(true);
}



void fragment(hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< bc_packet > &meta_out,
		hls::stream< c_data_t > &out,
		hls::stream< bool > &end_out){
#pragma HLS DATAFLOW
	hls::stream< ap_uint< 8 > , MAX_BIG_CHUNK_SIZE/8 > segment_data("segment_data");
	hls::stream< c_size_t , 2 > segment_size("segment_size");
	hls::stream< bool , 2 > segment_end("segment_end");

	hls::stream< ap_uint< 8 > , MAX_BIG_CHUNK_SIZE/8 > write_data("write_data");
	hls::stream< c_size_t , 2 > write_size("write_size");
	hls::stream< bool , 2 > write_end("write_end");

	convert_to_byte_stream(in, size_in, end_in, segment_data, segment_size, segment_end);

	segment_bc_packet(segment_data, segment_size, segment_end, write_data, write_size, write_end);

	write_out(write_data, write_size, write_end, meta_out, out, end_out);
}
