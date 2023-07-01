/*
 * This file contains the reorder kernel of the dedup toolchain
 */

#include "reorder.hpp"


/*
 * Writes the header to the output stream
 *
 * @param out: pointer to the output stream
 */
static void write_header(hls::stream< ap_uint< 8 > > &out){
	ap_uint< 32 > checkbit = CHECKBIT;
	write_header: for (int i = 0 ; i < 4 ; i++){ //TODO assume LSB first to send
#pragma HLS UNROLL
		out.write(checkbit.range(7 + i*8, i*8));
	}
	out.write(COMPRESS_NONE);
}


/*
 * Writes the seperator between chunks
 *
 * @param type: MACRO type of chunk which follows
 * @param size: size of the chunk that follows
 * @param out : pointer to the output stream
 */
static void write_seperator(ap_uint< 8 > type, c_size_t size, hls::stream< ap_uint< 8 > > &out){
	out.write(type);
	write_size_of_chunk_to_file: for (int i = 0 ; i < W_CHUNK_SIZE/8 ; i++){
#pragma HLS UNROLL
		out.write(size.range(7 + i*8, i*8));
	}
}



static void read_in(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		c_data_t *in_data_buffer,
		sc_packet &current){
	sc_packet meta = meta_in.read();
	current = sc_packet(meta);

	int i;
	read_in_data: for (i = 0 ; i < hls::ceil((double) meta.size.to_long()*8 / W_DATA) ; i++){
		in_data_buffer[i] = data_in.read();
	}

	set_to_zero: for ( ; i < SC_STREAM_SIZE ; i++){
		in_data_buffer[i] = 0;
	}
}



static void check_next_packet(l1_pos_t l1_pos, l2_pos_t l2_pos, sc_packet current, bool &is_next){
	is_next = l1_pos == current.l1_pos && l2_pos == current.l2_pos;
}



/*
 * Writes out the data to the output FIFO for write back.
 *
 * @param in : small chunk packet to be written
 * @param out: pointer to output FIFO
 */
static void write_out(sc_packet &meta_in, c_data_t *data_in, hls::stream< ap_uint< 8 > > &out){
	if (meta_in.is_duplicate){
		//write seperator
		write_seperator(TYPE_FINGERPRINT, 20, out);

		//duplicate chunk -> write  hash to output
		write_hash_to_file: for (int j = 0 ; j < W_ADDR/8 ; j++){
	#pragma HLS UNROLL
			out.write(meta_in.hash.range( 7 + 8*j , 8*j ));
		}
	} else {
		//write seperator
		write_seperator(TYPE_COMPRESS, meta_in.size, out); //FIXME data is not compressed but PARSEC still writes TYPE_COMPRESS to file

		//unique chunk -> write chunk to output
		write_data_to_file: for (int i = 0 ; i < hls::ceil((double) meta_in.size.to_long()*8 / W_DATA) ; i++){
			c_data_t buffer = data_in[i];
			for (int j = 0 ; j < W_DATA/8 ; j++){
#pragma HLS UNROLL
				if (meta_in.size.to_long() > i*W_DATA/8 + j)
					out.write(buffer.range( 7 + 8*j , 8*j));
			}
		}
	}
}



/*
 * updates level 1 and level 2 positions which are next in order
 */
void update_pos(bool &last_l2_chunk, l1_pos_t &l1_pos, l2_pos_t &l2_pos){
	if (last_l2_chunk){
		l1_pos++;
		l2_pos = 0;
	} else {
		l2_pos++;
	}
}



/*
 * Reorder kernel of the dedup toolchain
 *
 * @param in : stream of unsorted small chunk packages
 * @param out: stream of bytes to write back
 *
 */
void reorder(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		bool end,
		hls::stream< ap_uint< 8 > > &out){
	//positions for the next chunk
	l1_pos_t l1_pos = 0;
	l2_pos_t l2_pos = 0;
	//buffer for storing chunks
	static buffer_cell buffer[BUFFER_SIZE_1][BUFFER_SIZE_2];

	//write header
	write_header(out);

	//reorder loop
	sc_packet read_current;
	sc_packet bram_current;
	c_data_t data_in_buffer[SC_STREAM_SIZE];
	c_data_t bram_data_buffer[SC_STREAM_SIZE];
	bool chunk_in_buffer = false;
	reorder_loop: while(!meta_in.empty() || !end || chunk_in_buffer) {
		//get next item from queue
		if (!meta_in.empty()){
			read_in(meta_in, data_in, data_in_buffer, read_current);

			//check if this is the next chunk
			bool is_next_chunk;
			check_next_packet(l1_pos, l2_pos, read_current, is_next_chunk);

			if (is_next_chunk){
				//write chunk to the output stream
				update_pos(read_current.last_l2_chunk, l1_pos, l2_pos);

				write_out(read_current, data_in_buffer, out);
			} else {
				//store to buffer
				write_buffer(read_current, data_in_buffer, buffer);
			}
		}

		//check the buffer contains the next package
		read_buffer(l1_pos, l2_pos, buffer, bram_current, bram_data_buffer, chunk_in_buffer);
		if (chunk_in_buffer){
			//update next chunk positions
			update_pos(bram_current.last_l2_chunk, l1_pos, l2_pos);

			//write to output stream
			write_out(bram_current, bram_data_buffer, out);
		}
	}
}
