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
#define MIN_BC_SIZE BIG_CHUNK_SIZE



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

	static bool init_write = true;
#pragma HLS RESET variable=init_write
	static l1_pos_t l1;
#pragma HLS RESET variable=l1

	if (end_in.empty())
		return;

	//hls::print("\tWrite out.\n");

	if (end_in.read()){
		end_out.write(true);
		return;
	}

	//Initialization and checks
	if (init_write){
		l1 = 0;
		init_write = false;
	}


	//pasing of data
	end_out.write(false);

	//write meta data
	bc_packet tmp_meta;
	tmp_meta.l1_pos = l1++;
	c_size_t chunk_length = size_in.read();
	tmp_meta.size = chunk_length;
	meta_out.write(tmp_meta);

	//write data
	write_chunk: for (c_size_t i = 0 ; i < BC_STREAM_SIZE ; i++){
#pragma HLS LOOP_FLATTEN off
		if (i.to_int() >= hls::ceil((double) chunk_length.to_long()*8 / W_DATA))
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
	fill_buffer_loop: for (int i = 0 ;  i < MIN_BC_SIZE / 8 ; i++){
#pragma HLS PIPELINE II=1
		if (file_length_buffer <= 0){
			break;
		}
		out.write(in.read());
		read_counter++;
		file_length_buffer--;
	}

	file_length = file_length_buffer;
	read = read_counter;
}


/*
 * @brief segment the byte stream according to rabin fingerprint
 */
static void segment_bc_packet(
		hls::stream< ap_uint< 8 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > & end_in,
		hls::stream< ap_uint< 8 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out){
	static unsigned rabintab[256], rabinwintab[256];
#pragma HLS BIND_STORAGE variable=rabintab type=ram_2p
#pragma HLS BIND_STORAGE variable=rabinwintab type=ram_2p

	static bool end_seg, init_seg = true;
#pragma HLS RESET variable=end_seg
#pragma HLS RESET variable=init_seg
	static c_size_t file_length_seg;
#pragma HLS RESET variable=file_length_seg


	if(!end_seg){
		//hls::print("\tSegment bc packet\n");
		if (init_seg){
			if (end_in.empty())
				return;

			if (end_in.read()){
				end_seg = true;
				end_out.write(true);
				return;
			}

			rabininit(rabintab, rabinwintab);

			file_length_seg = size_in.read();
			init_seg = false;
		}

		end_out.write(false);

		c_size_t bc_size;
		fill_buffer(in, file_length_seg, bc_size, out);

		c_size_t sc_size;
		rabinseg_in_stream(in, file_length_seg, out, sc_size, rabintab, rabinwintab);

		size_out.write(sc_size + bc_size);

		if (file_length_seg <= 0){
			init_seg =  true;
		}
	}
}


/*
 * @brief Convert the c_data input stream into byte stream
 */
static void convert_to_byte_stream(
		hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< ap_uint< 8 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out){

	static bool end_byte, init_byte = true;
#pragma HLS RESET variable=end_byte
#pragma HLS RESET variable=init_byte
	static c_size_t file_length_byte;
#pragma HLS RESET variable=file_length_byte
	static c_size_t input_counter_byte;
#pragma HLS RESET variable=input_counter_byte
	static ap_uint< 64 > current_long;
#pragma HLS RESET variable=current_long
	static int pos_byte;
#pragma HLS RESET variable=pos_byte


	if (!end_byte || out.full()){
		//hls::print("\tConvert to byte stream.\n");
		if (init_byte){
			if (end_in.empty() || end_out.full())
				return;

			if (end_in.read()){
				end_byte = true;
				end_out.write(true);
				return;
			}

			file_length_byte = size_in.read();
			input_counter_byte = 0;

			end_out.write(false);
			size_out.write(file_length_byte);
			init_byte = false;
			pos_byte = 0;
		}

		read_data: for (c_size_t i = 0 ; i < MAX_BIG_CHUNK_SIZE/8 + 1 ; i+=8){
#pragma HLS PIPELINE II=8
			if (input_counter_byte + pos_byte >= file_length_byte)
				break;

			if (pos_byte == 0){
				current_long = in.read();
			}

			convert_to_byte: for (int j = 0 ; j < 8 ; j++){
				if (input_counter_byte + pos_byte < file_length_byte && pos_byte < 8){
					if (out.write_nb(current_long.range(7 + 8*pos_byte, 8*pos_byte))){
						pos_byte++;
					}
				}
			}

			if (pos_byte >= 8){
				input_counter_byte += 8;
				pos_byte = 0;
			}
		}

		if (input_counter_byte + pos_byte >= file_length_byte){
			init_byte = true;
		}
	}
}



void fragment(hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< bc_packet > &meta_out,
		hls::stream< c_data_t > &out,
		hls::stream< bool > &end_out){
	static hls::stream< ap_uint< 8 > , MAX_FILE_SIZE/8 > 		segment_data("segment_data");
	static hls::stream< c_size_t , 2 > 							segment_size("segment_size");
	static hls::stream< bool , 2 > 								segment_end("segment_end");

	hls::stream< ap_uint< 8 > , MAX_BIG_CHUNK_SIZE/8 > 			write_data("write_data");
	hls::stream< c_size_t , 2 > 								write_size("write_size");
	static hls::stream< bool , 2 > 								write_end("write_end");

	//hls::print("Fragment kernel:\n");

	convert_to_byte_stream(in, size_in, end_in, segment_data, segment_size, segment_end);

	segment_bc_packet(segment_data, segment_size, segment_end, write_data, write_size, write_end);

	write_out(write_data, write_size, write_end, meta_out, out, end_out);
}
