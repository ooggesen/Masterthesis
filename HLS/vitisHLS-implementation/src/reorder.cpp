/*
 * @file reorder.cpp
 *
 * @brief Contains the reorder pipeline stage
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "reorder.hpp"


/**
 * @brief Writes seperator between small chunks to the output
 */
static void write_seperator(ap_uint< 64 > type, c_size_t size, hls::stream< ap_uint< 64 > > &out){
	out.write(type);
	out.write(size);
}



/**
 * @brief Writes small chunk to output of reorder pipeline stage
 */
static void write_out(sc_packet &meta_in, c_data_t *data_in, hls::stream< ap_uint< 64 > > &out){
	if (meta_in.is_duplicate){
		//write seperator
		write_seperator(TYPE_FINGERPRINT, 20, out);

		//duplicate chunk -> write  hash to output
		out.write(meta_in.hash.range(63, 0));
		out.write(meta_in.hash.range(127, 64));
		out.write(meta_in.hash.range(159, 128));
	} else {
		//write seperator
		write_seperator(TYPE_COMPRESS, meta_in.size, out); //FIXME data is not compressed but PARSEC still writes TYPE_COMPRESS to file

		//unique chunk -> write chunk to output
		write_data_to_file: for (c_size_t i = 0 ; i < SC_STREAM_SIZE ; i++){
#pragma HLS PIPELINE II=16
			if (i.to_long() >= hls::ceil((double) meta_in.size.to_long()*8 / W_DATA))
				break;

			segment_c_data_to_long: for (int j = 0 ; j < W_DATA/64 ; j++){
				if (meta_in.size.to_long() > i.to_long()*W_DATA/8 + j*8)
					out.write(data_in[i].range( 63 + 64*j , 64*j));
			}
		}
	}
}



/**
 * @brief Reads data in
 */
static void read_in(
		hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		c_data_t *in_data_buffer,
		sc_packet &current){
	sc_packet meta = meta_in.read();
	current = sc_packet(meta);

	read_in_data: for (c_size_t i = 0 ; i < SC_STREAM_SIZE ; i++){
		if (i.to_long() < hls::ceil((double) meta.size.to_long()*8 / W_DATA))
			in_data_buffer[i] = data_in.read();
		else
			in_data_buffer[i] = 0;
	}
}


/**
 * @brief Writes header of output stream with file informations.
 */
static void write_header(hls::stream< ap_uint< 64 > > &out){
	out.write(CHECKBIT);
	out.write(COMPRESS_NONE);
}



/**
 * @brief Checks the input for the next small chunk according to level 1 and level 2 positions.
 */
static void check_input(
		hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool > &end_in,
		c_size_t &file_length,
		bool &end,
		hls::stream< ap_uint< 64 > > &out){
	sc_packet read_current;
	c_data_t data_in_buffer[SC_STREAM_SIZE];
#pragma HLS ARRAY_PARTITION variable=data_in_buffer type=complete

	if (!end_in.empty()){
		if (!end_in.read()){
			read_in(meta_in, data_in, data_in_buffer, read_current);

			//add output length to total file length
			if (read_current.is_duplicate)
				file_length += 40; // 160 bit(20 byte) hash + 4 byte zero stuffing + 8 byte chunk type + 8 byte size
			else{
				file_length += read_current.size + 16; // size + 8 byte chunk type + 8 byte size + max of 7 byte zero stuffing
				if (read_current.size % 8 != 0){
					file_length += 8 - (read_current.size % 8); // account for zero stuffing
				}
			}

			write_out(read_current, data_in_buffer, out);
		} else {
			end = true;
		}
	}
}




void reorder(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool > &end_in,
		hls::stream< c_size_t > &size_out,
		hls::stream< ap_uint< 64 > > &data_out,
		hls::stream< bool > &end_out){
	//positions for the next chunk
	//file length buffer
	static c_size_t file_length = 0;
#pragma HLS RESET variable=file_length

	static bool end_r = false, init = true;

	//initializations
	if (init){
		init = false;
		end_out.write(false);
		write_header(data_out);

		file_length = 16; //8 byte checkbit plus 8 byte compress type information in header
	}


	//parsing of small chunks
	check_input(meta_in, data_in, end_in, file_length,
			end_r, data_out);


	//end of process condition
	if (end_r){
		end_out.write(true);
		size_out.write(file_length);
	}
}
