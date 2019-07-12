/*
 * This file contains the kernel for the coarse grained chunking algorithm
 */

#include "fragment.hpp"



#define MIN_BC_SIZE (BIG_CHUNK_SIZE - 10*SMALL_CHUNK_SIZE)



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
		c_size_t chunk_length = size_in.read();

		write_chunk: for (int i = 0 ; i < hls::ceil((double) chunk_length.to_long()*8 / W_DATA) ; i++){
			c_data_t buffer;
			convert_to_c_data_t: for (c_size_t j = 0 ; j < W_DATA/8 ; j++){
				if (chunk_length.to_long() > i*W_DATA/8 + j)
					buffer.range(7 + 8*j , 8*j) = in.read();
				else
					buffer.range(7 + 8*j , 8*j) = 0;
			}
			data_out.write(buffer);
		}

		bc_packet tmp_meta;
		tmp_meta.l1_pos = l1++;
		tmp_meta.size = chunk_length;
		meta_out.write(tmp_meta);

		end_out.write(false);
		end = end_in.read();
	}

	end_out.write(true);
}



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
#pragma HLS BIND_STORAGE variable=rabintab type=ROM_1P
#pragma HLS BIND_STORAGE variable=rabinwintab type=ROM_1P
	rabininit(rabintab, rabinwintab);

	bool end = end_in.read();

	while(!end){
#pragma HLS PIPELINE off
		c_size_t file_length = size_in.read();

		segment_file: while(file_length > 0){
			c_size_t bc_size;
			fill_buffer(in, file_length, bc_size, out);

			c_size_t sc_size;
			rabinseg_in_stream(in, file_length, out, sc_size, rabintab, rabinwintab);

			size_out.write(sc_size + bc_size);

			end_out.write(false);
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
		c_size_t file_length = size_in.read();

		read_data: for (c_size_t i = 0 ; i < (int) MAX_FILE_SIZE/8 ; i++){
			if (i*8 >= file_length)
				break;

			ap_uint< 64 > current_long = in.read();
			convert_to_byte: for (int j = 0 ; j < 8 ; j++){
				if (i*8 + j < file_length)
					out.write(current_long.range(7 + 8*j, 8*j));
			}
		}

		end_of_file:
		end_out.write(false);
		size_out.write(file_length);

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
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
#pragma HLS DATAFLOW
	hls::stream< ap_uint< 8 > , 16 > segment_data("segment_data");
	hls::stream< c_size_t , 2 > segment_size("segment_size");
	hls::stream< bool , 2 > segment_end("segment_end");

	hls::stream< ap_uint< 8 > , 16 > write_data("write_data");
	hls::stream< c_size_t , 2 > write_size("write_size");
	hls::stream< bool , 2 > write_end("write_end");

	convert_to_byte_stream(in, size_in, end_in, segment_data, segment_size, segment_end);

	segment_bc_packet(segment_data, segment_size, segment_end, write_data, write_size, write_end);

	write_out(write_data, write_size, write_end, meta_out, out, end_out);
}
