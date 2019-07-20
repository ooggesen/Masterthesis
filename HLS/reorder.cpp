/*
 * @file reorder.cpp
 *
 * @brief Contains the reorder pipeline stage
 *
 * @author Ole Oggesen
 * @bug Reorder stage assumes an empty buffer before the arrival of a new file.
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
 * @brief Updates level 1 and level 2 positions of the small chunk in the file.
 */
static void update_pos(bool &last_l2_chunk, l1_pos_t &l1_pos, l2_pos_t &l2_pos){
	if (last_l2_chunk){
		l1_pos++;
		l2_pos = 0;
	} else {
		l2_pos++;
	}
}



/**
 * @brief Checks the buffer for the next small chunk according to level 1 and level 2 positions
 */
static void check_buffer(l1_pos_t &l1, l2_pos_t &l2, buffer_cell buffer[][BUFFER_SIZE_2], int &buffer_counter, hls::stream< ap_uint< 64 > > &out){
	sc_packet bram_current;
	c_data_t bram_data_buffer[SC_STREAM_SIZE];
#pragma HLS ARRAY_PARTITION variable=bram_data_buffer type=complete

	bool chunk_in_buffer;
	read_buffer(l1, l2, buffer, bram_current, bram_data_buffer, chunk_in_buffer, buffer_counter);
	if (chunk_in_buffer){
		//update next chunk positions
		update_pos(bram_current.last_l2_chunk, l1, l2);

		//write to output stream
		write_out(bram_current, bram_data_buffer, out);
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
		l1_pos_t &l1,
		l2_pos_t &l2,
		c_size_t &file_length,
		buffer_cell buffer[][BUFFER_SIZE_2],
		int &buffer_counter,
		bool &end,
		hls::stream< c_size_t > &size_out,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< bool > &end_out){
	sc_packet read_current;
	c_data_t data_in_buffer[SC_STREAM_SIZE];
#pragma HLS ARRAY_PARTITION variable=data_in_buffer type=complete

	if (!end_in.empty()){
		if (!end_in.read()){
			read_in(meta_in, data_in, data_in_buffer, read_current);

			//at start of file
			if (read_current.l1_pos == 0 && read_current.l2_pos == 0){
				write_header(out);
				end_out.write(false);

				l1 = 0;
				l2 = 0;

				//not on the initial run
				if (file_length != 0){
					size_out.write(file_length);
				}

				file_length = 16; //8 byte checkbit plus 8 byte compress type information
			}

			//add output length to total file length
			if (read_current.is_duplicate)
				file_length += 40; // 160 bit(20 byte) + 4 byte zero stuffing + 8 byte chunk type + 8 byte size
			else{
				file_length += read_current.size + 16; // size + 8 byte chunk type + 8 byte size + max of 8 byte zero stuffing
				if (read_current.size % 8 != 0){
					file_length += 8 - (read_current.size % 8); // account for zero stuffing
				}
			}

			//check if this is the next chunk
			if (l1 == read_current.l1_pos && l2 == read_current.l2_pos){
				//write chunk to the output stream
				update_pos(read_current.last_l2_chunk, l1, l2);

				write_out(read_current, data_in_buffer, out);
			} else {
				write_buffer(read_current, data_in_buffer, buffer, buffer_counter);
			}
		} else {
			end = true;
			size_out.write(file_length);
			end_out.write(true);
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
	l1_pos_t l1_pos;
	l2_pos_t l2_pos;
	//file length buffer
	c_size_t file_length = 0;
	//buffer for storing chunks
	buffer_cell buffer[BUFFER_SIZE_1][BUFFER_SIZE_2];
#pragma HLS BIND_STORAGE variable=buffer type=ram_2p

	//reorder loop
	bool end = false;
	int buffer_counter = 0;
	reorder_loop: while(!end || buffer_counter != 0) {
#pragma HLS PIPELINE off
#pragma HLS LOOP_FLATTEN off
		check_input(meta_in, data_in, end_in, l1_pos, l2_pos, file_length, buffer, buffer_counter,
				end, size_out, data_out, end_out);

		check_buffer(l1_pos, l2_pos, buffer, buffer_counter, data_out);
	}
}
