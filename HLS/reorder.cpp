/*
 * This file contains the reorder kernel of the dedup toolchain
 */

#include "reorder.h"

void write_buffer(bus_packet to_write, stack &buffer){
	if (buffer->sp < STACK_DEPTH){
		copy(to_write, buffer.stack[buffer.sp]);
		buffer->sp++;
	}
}

bool read_buffer(l1_pos_t l1, l2_pos_t l2, stack &buffer, bus_packet &out){
	for (int elem = buffer.sp-1 ; elem >= 0 ; elem--){
		bus_packet *bp = &buffer.stack[elem];
		if (bp->l1_pos == l1 && bp->l2_pos == l2){
			copy(*bp, out);
			return true;
		}
	}
	return false;
}

void write_out(bus_packet in, hls::stream< ap_uint< 32 > > &out){
	if (in.is_duplicate){
		//duplicate; write  hash to output
		for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
	#pragma HLS UNROLL
			for (int j = 0 ; j < W_ADDR / 32 ; j++){
	#pragma HLS UNROLL
				out.write(in.hash.range( 31 + 32 * j , 32 * j ));
			}
		}
	} else {
		//unique chunk write chunk to output
		for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
	#pragma HLS UNROLL
			for (int j = 0 ; j < W_DATA_SMALL_CHUNK / 32 ; j++){
	#pragma HLS UNROLL
				out.write(in.data[i].range( 31 + 32 * j , 32 * j ));
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
 * TODO end of stream not yet implemented
 */
// Assemble the output stream
void reorder(hls::stream< bus_packet > &in, hls::stream< ap_uint< 32 > > &out){
	//positions for the next chunk
	static l1_pos_t l1_pos = 0;
	static l2_pos_t l2_pos = 0;

	//buffer for storing chunks
	static stack buffer;


	bus_packet current;
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
		while (read_buffer(l1_pos, l2_pos, buffer, current)){
			update_pos(current.last_l2_chunk, l1_pos, l2_pos);

			write_out(current, out);
		}
	} while(!current.end);
}
