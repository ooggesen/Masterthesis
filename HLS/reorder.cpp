/*
 * This file contains the reorder kernel of the dedup toolchain
 */

#include "reorder.hpp"

void write_header(hls::stream< ap_uint< 8 > > &out){
	ap_uint< 32 > checkbit = CHECKBIT;
	write_header: for (int i = 0 ; i < 4 ; i++){ //TODO assume MSB first to send
#pragma HLS UNROLL
		out.write(checkbit.range(31 - i*8, 24 - i*8));
	}
	out.write(COMPRESS_NONE);
}

void write_seperator(ap_uint< 8 > type, c_size_t &size, hls::stream< ap_uint< 8 > > &out){
	out.write(type);
	write_size_of_chunk_to_file: for (int i = 0 ; i < W_CHUNK_SIZE / 8 ; i++){
#pragma HLS UNROLL
		out.write(size.range(W_CHUNK_SIZE-1 - i*8, W_CHUNK_SIZE-8 - i*8));
	}
}

void write_out(sc_packet &in, hls::stream< ap_uint< 8 > > &out){
	if (in.is_duplicate){
		//write seperator
		write_seperator(TYPE_FINGERPRINT, in.size, out);

		//duplicate; write  hash to output
		write_hash_to_file: for (int j = 0 ; j < W_ADDR / 8 ; j++){
	#pragma HLS UNROLL
			out.write(in.hash.range( W_ADDR-1 - 8*j , W_ADDR-8 - 8*j ));//TODO assume MSB first to send
		}
	} else {
		//write seperator
		write_seperator(TYPE_COMPRESS, in.size, out); //TODO data is not compressed but PARSEC still writes TYPE_COMPRESS to file

		//unique chunk write chunk to output
		write_data_to_file: for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
	#pragma HLS UNROLL
			for (int j = 0 ; j < W_DATA_SMALL_CHUNK / 8 ; j++){
	#pragma HLS UNROLL
				out.write(in.data[i].range( W_DATA_SMALL_CHUNK-1 - 8*j , W_DATA_SMALL_CHUNK-8 - 8*j ));
			}
		}
	}
}

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
 * @param in : stream of unsorted small chunk packages
 * @param out: stream of bytes to write back
 *
 * TODO end of stream not yet implemented
 */
void reorder(hls::stream< sc_packet > &in, hls::stream< ap_uint< 8 > > &out){
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
	} while(!current.end);

	return;
}
