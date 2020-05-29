/*
 * This file contains the dedup kernel for the dedup toolchain
 *
 */


#include "dedup.h"


/*
 * Function for flushing (emtying) one FIFO buffer
 */
template <typename T, int w>
void flush_buffer(hls::stream<T, w> &buffer){
	flush_buffer:
	while(!buffer.empty())buffer.read();
}



/*
 * Flushing all the buffers
 */
void reset_buffers(	hls::stream< ap_uint< 32 > , MSG_BUFF_SIZE > &sha1_msg,
					hls::stream< ap_uint< 64 > , 2 > &sha1_len,
					hls::stream< bool , 2 > &sha1_end_len,
					hls::stream< ap_uint< 160 > , 2 > &sha1_digest,
					hls::stream< bool , 2 > &sha1_end_digest){
	flush_buffer< ap_uint< 32 >, MSG_BUFF_SIZE >(sha1_msg);
	flush_buffer< ap_uint< 64 >, 2 >(sha1_len);
	flush_buffer< bool, 2>(sha1_end_len);
	flush_buffer< ap_uint< 160 >, 2 >(sha1_digest);
	flush_buffer< bool, 2 >(sha1_end_digest);
}



/*
 * Function for performing the read of the interface to the FIFO streams
 */
void read_in(bus_packet in,
		hls::stream< ap_uint< 32 > , MSG_BUFF_SIZE > &sha1_msg,
		hls::stream< ap_uint< 64 > , 2 > &sha1_len,
		hls::stream< bool , 2 > &sha1_end_len)
{
	sha1_end_len.write(false);

	sha1_len.write((unsigned long long) in.size);

	read_data:
	for (int i = 0; i < SC_ARRAY_SIZE; i++){
		sc_data_t current = in.data[i];
		//unsigned current_int = current.to_int();

		int size_remaining = in.size * 8 - i * W_DATA_SMALL_CHUNK;
		if (size_remaining > 0){
			write_to_sha1_buff:
			for (int j = 0; j < W_DATA_SMALL_CHUNK / 32 ; j++){
				// break after the last package in stream
				if (size_remaining - j * 32 <= 0){
					goto write_end_stream;
				}

				sha1_msg.write(current.range(31 + 32 * j, 32 * j));
				//unsigned tmp = current.range(31 + 32 * j, 32 * j).to_int();
			}
		}
	}

	write_end_stream:
	sha1_end_len.write(true);

}


void convert_to_bram(bus_packet in, bram_packet &to_bram){
	to_bram.addr = in.hash;
	copy_to_bram_loop:
	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
#pragma HLS UNROLL
		to_bram.data[i] = in.data[i];
	}
}

bool check_duplicate(bram_packet to_bram){
	bram_packet packet_r;
	packet_r.addr = to_bram.addr;

	bram(false, true, to_bram, packet_r);

	if (is_equal(to_bram.data, packet_r.data)){
		return true; //found duplicate
	} else {
		//found unique chunk
		bram(true, false, to_bram, packet_r); //write to bram
		return false;
	}
}



/*
 * main function for dedup kernel
 *
 * @param in[N] : array of bus_packet type containing a whole small chunk
 * @param to_bram : interface to the BRAM modules
 * @param to_compress: interface to the compression stage, used if chunk is NOT a duplicate
 * @param to_reorder: interface to the reorder stage, used if chunk is a duplicate
 *
 * TODOs:
 * 	.in.end not yet implemented
 */
void dedup(bus_packet in, bus_packet &out){
#pragma HLS PIPELINE
	//sha1 streams
	hls::stream< ap_uint< 32 > , MSG_BUFF_SIZE > sha1_msg("sha1_msg");
	hls::stream< ap_uint< 64 > , 2 > sha1_len("sha1_len");
	hls::stream< bool , 2 > sha1_end_len("sha1_end_len");
	hls::stream< addr_t , 2 > sha1_digest("sha1_digest");
	hls::stream< bool , 2 > sha1_end_digest("sha1_end_digest");

	//flush the buffers
	//reset_buffers(sha1_msg, sha1_len, sha1_end_len, sha1_digest, sha1_end_digest);

	//move data to streams
	read_in(in, sha1_msg, sha1_len, sha1_end_len);

	//calculate sha1 hash
	xf::security::sha1< 32 >(sha1_msg, sha1_len, sha1_end_len, sha1_digest, sha1_end_digest);

	//create bus packet
	bram_packet to_bram;
	bool sha1__end = sha1_end_digest.read(); //FIXME: not yet used
	in.hash = sha1_digest.read();
	convert_to_bram(in, to_bram);

	//check for duplicate
	in.is_duplicate = check_duplicate(to_bram);

	//pass to next stage
	copy(in, out);
}


