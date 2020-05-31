/*
 * This file contains the reorder kernel of the dedup toolchain
 */

#include "reorder.h"

void write_header(hls::stream< ap_uint< 8 > > &out){
	ap_uint< 32 > checkbit = CHECKBIT;
	for (int i = 0 ; i < 4 ; i++){ //TODO assume MSB first to send
		out.write(checkbit.range(31 - i*8, 24 - i*8));
	}
	out.write(COMPRESS_NONE); //TODO in parsec this is a byte not a 32 bit u_integer_t
}

void write_buffer(bus_packet to_write, stack &buffer){
	if (buffer.sp < STACK_DEPTH){
		copy(to_write, buffer.stack[buffer.sp]);
		buffer.sp++;
	}
}

void read_buffer(l1_pos_t l1, l2_pos_t l2, stack &buffer, bus_packet &out, bool &success){
	if (buffer.sp != 0){
		check_buffer_for_next_chunk:
		for (int elem = buffer.sp-1 ; elem >= 0 ; elem--){
			bus_packet *bp = &buffer.stack[elem];
			if (bp->l1_pos == l1 && bp->l2_pos == l2){
				copy(*bp, out);
				buffer.sp--;
				success = true;
				return;
			}
		}
	}
	success = false;;
}

void write_seperator(ap_uint< 8 > type, c_size_t size, hls::stream< ap_uint< 8 > > &out){
	out.write(type);
	write_size_of_chunk_to_file:
	for (int i = 0 ; i < W_CHUNK_SIZE / 8 ; i++){
#pragma HLS UNROLL
		out.write(size.range(W_CHUNK_SIZE-1 - i*8, W_CHUNK_SIZE-8 - i*8));
	}
}

void write_out(bus_packet in, hls::stream< ap_uint< 8 > > &out){
	if (in.is_duplicate){
		//write seperator
		write_seperator(TYPE_FINGERPRINT, in.size, out);

		//duplicate; write  hash to output
		write_chunk_to_file:
		for (int j = 0 ; j < W_ADDR / 8 ; j++){
	#pragma HLS UNROLL
			out.write(in.hash.range( W_ADDR-1 - 8*j , W_ADDR-8 - 8*j ));//TODO assume MSB first to send
		}
	} else {
		//write seperator
		write_seperator(TYPE_COMPRESS, in.size, out); //TODO data is not compressed but PARSEC still writes TYPE_COMPRESS to file

		//unique chunk write chunk to output
		for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
	#pragma HLS UNROLL
			for (int j = 0 ; j < W_DATA_SMALL_CHUNK / 8 ; j++){
	#pragma HLS UNROLL
				out.write(in.data[i].range( W_DATA_SMALL_CHUNK-1 - 8*j , W_DATA_SMALL_CHUNK-8 - 8*j ));
			}
		}
	}
}

void update_pos(bool last_l2_chunk, l1_pos_t &l1_pos, l2_pos_t &l2_pos){
	if (last_l2_chunk){
		l1_pos++;
		l2_pos = 0;
	} else {
		l2_pos++;
	}
}

/*
 * Reorder kernel of the dedup toolchain
 * TODO end of stream not yet implemented
 */
// Assemble the output stream
void reorder(hls::stream< bus_packet > &in, hls::stream< ap_uint< 8 > > &out){
	//positions for the next chunk
	l1_pos_t l1_pos = 0;
	l2_pos_t l2_pos = 0;

	//buffer for storing chunks
	stack buffer;

	//write header
	write_header(out);

	//reorder loop
	bus_packet current;
	reorder_loop:
	do {
		//get next item from queue
		current = in.read();

		//check if this is the next chunk
		bool is_next_chunk = l1_pos == current.l1_pos && l2_pos == current.l2_pos;

		if (is_next_chunk){
			//write chunk to the ouput stream
			update_pos(current.last_l2_chunk, l1_pos, l2_pos);

			write_out(current, out);
		} else {
			//store to buffer
			write_buffer(current, buffer);
		}

		//check the buffer contains the next package
		bool chunk_in_buffer = false;
		read_buffer(l1_pos, l2_pos, buffer, current, chunk_in_buffer);
		while (chunk_in_buffer){
			//update next chunk positions
			update_pos(current.last_l2_chunk, l1_pos, l2_pos);

			//write to output stream
			write_out(current, out);

			//check again for next chunk in buffer
			read_buffer(l1_pos, l2_pos, buffer, current, chunk_in_buffer);
		}
	} while(!current.end);
}
