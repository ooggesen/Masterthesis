/*
 * This file contains the reorder kernel of the dedup toolchain
 */

#include "reorder.hpp"


/*
 * Writes the header to the output stream
 *
 * @param out: pointer to the output stream
 */
void write_header(hls::stream< ap_uint< 8 > > &out){
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
void write_seperator(ap_uint< 8 > type, c_size_t &size, hls::stream< ap_uint< 8 > > &out){
	out.write(type);
	write_size_of_chunk_to_file: for (int i = 0 ; i < W_CHUNK_SIZE / 8 ; i++){
#pragma HLS UNROLL
		out.write(size.range(7 + i*8, i*8)); //TODO assume LSB first
	}
}



/*
 * Writes out the data to the output FIFO for write back.
 *
 * @param in : small chunk packet to be written
 * @param out: pointer to output FIFO
 */
void write_out(sc_packet &in, hls::stream< ap_uint< 8 > > &out){
	if (in.is_duplicate){
		//write seperator
		write_seperator(TYPE_FINGERPRINT, in.size, out);

		//duplicate chunk -> write  hash to output
		write_hash_to_file: for (int j = 0 ; j < W_ADDR / 8 ; j++){
	#pragma HLS UNROLL
			out.write(in.hash.range( 7 + 8*j , 8*j ));//TODO assume MSB first to send
		}
	} else {
		//write seperator
		write_seperator(TYPE_COMPRESS, in.size, out); //FIXME data is not compressed but PARSEC still writes TYPE_COMPRESS to file

		//unique chunk -> write chunk to output
		write_data_to_file: for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
	#pragma HLS UNROLL
			for (int j = 0 ; j < W_DATA_SMALL_CHUNK / 8 ; j++){
	#pragma HLS UNROLL
				out.write(in.data[i].range( 7 + 8*j , 8*j ));
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
void reorder(hls::stream< sc_packet > &in, bool end, hls::stream< ap_uint< 8 > > &out){
	//positions for the next chunk
	l1_pos_t l1_pos = 0;
	l2_pos_t l2_pos = 0;

	//buffer for storing chunks
	static buffer_cell buffer[BUFFER_SIZE_1][BUFFER_SIZE_2];

	//write header
	write_header(out);

	//reorder loop
	sc_packet current;
	unsigned i = 0;
	reorder_loop: do {
		i++;
		//get next item from queue
		if (!in.empty()){
			current = in.read();

			//check if this is the next chunk
			bool is_next_chunk = l1_pos == current.l1_pos && l2_pos == current.l2_pos;

			if (is_next_chunk){
				//write chunk to the output stream
				update_pos(current.last_l2_chunk, l1_pos, l2_pos);

				write_out(current, out);
			} else {
				//store to buffer
				write_buffer(current, buffer);

				//reset buffer
				current = sc_packet();
			}
		}

		//check the buffer contains the next package
		bool chunk_in_buffer = false;
		read_buffer(l1_pos, l2_pos, buffer, current, chunk_in_buffer);
		chunk_in_buffer: while (chunk_in_buffer){
			//update next chunk positions
			update_pos(current.last_l2_chunk, l1_pos, l2_pos);

			//write to output stream
			write_out(current, out);

			//check again for next chunk in buffer
			read_buffer(l1_pos, l2_pos, buffer, current, chunk_in_buffer);
		}
	} while(!in.empty() || !end);
}
