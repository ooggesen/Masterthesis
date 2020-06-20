/*
 * This file contains the dedup kernel for the dedup toolchain
 *
 */


#include "dedup.hpp"


/*
 * Function for flushing (emtying) one FIFO buffer
 */
template <typename T, int w>
static void flush_buffer(hls::stream<T, w> &buffer){
	flush_buffer: while(!buffer.empty())buffer.read();
}



/*
 * Flushing all the buffers
 */
static void reset_buffers(	hls::stream< ap_uint< 32 > , MSG_BUFF_SIZE > &sha1_msg,
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
static void read_in(hls::stream< sc_packet > &in,
		bool end,
		hls::stream< sc_packet > &in_buffer,
		hls::stream< ap_uint< 32 > , MSG_BUFF_SIZE > &sha1_msg,
		hls::stream< ap_uint< 64 > , 2 > &sha1_len,
		hls::stream< bool , 2 > &sha1_end_len)
{
	while (!in.empty() || !end){
		if (!in.empty()){
			//write buffer
			sc_packet tmp_packet = in.read();
#pragma HLS ARRAY_PARTITION variable=tmp_packet type=complete
			in_buffer.write(tmp_packet);

			//write sha streams
			sha1_end_len.write(false);

			sha1_len.write((unsigned long long) tmp_packet.size);

			read_data: for (int i = 0; i < SC_ARRAY_SIZE; i++){
#pragma HLS UNROLL
				sc_data_t current = tmp_packet.data[i];

				int size_remaining = tmp_packet.size * 8 - i * W_DATA_SMALL_CHUNK;
				if (size_remaining > 0){
					write_to_sha1_buff: for (int j = 0; j < W_DATA_SMALL_CHUNK / 32 ; j++){
#pragma HLS UNROLL
						// stop after the last package in stream
						if (size_remaining - j * 32 > 0){
							sha1_msg.write(current.range(31 + 32 * j, 32 * j));
						}
					}
				}
			}
		}
	}

	//write end of stream for sha1 function
	sha1_end_len.write(true);
}


static void convert_to_bram(sc_packet in, bram_packet &to_bram){
#pragma HLS ARRAY_PARTITION variable=in.data type=complete
	to_bram.addr = in.hash;
	copy_to_bram: for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
#pragma HLS UNROLL
		to_bram.data[i] = in.data[i];
	}
}

static void check_duplicate(bram_packet to_bram, bool &is_duplicate){
	bram_packet packet_r;
	packet_r.addr = to_bram.addr;

	bram(false, true, to_bram, packet_r);

	if (is_equal(to_bram.data, packet_r.data)){
		is_duplicate = true; //found duplicate
	} else {
		//found unique chunk
		bram(true, false, to_bram, packet_r); //write to bram
		is_duplicate = false;
	}
}


static void write_out(sc_packet &packet, hls::stream< sc_packet > &out){
	out.write(packet);
}



/*
 * main function for dedup kernel
 *
 * @param in[N] : array of sc_packet type containing a whole small chunk
 * @param to_bram : interface to the BRAM modules
 * @param to_compress: interface to the compression stage, used if chunk is NOT a duplicate
 * @param to_reorder: interface to the reorder stage, used if chunk is a duplicate
 *
 * TODOs:
 * 	.in.end not yet implemented
 */
void dedup(hls::stream< sc_packet > &in, bool end, hls::stream< sc_packet > &out){
	//buffer
	hls::stream< sc_packet , 4> in_buffer("in_buffer");
	//sha1 streams
	hls::stream< ap_uint< 32 > , MSG_BUFF_SIZE > sha1_msg("sha1_msg");
	hls::stream< ap_uint< 64 > , 2 > sha1_len("sha1_len");
	hls::stream< bool , 2 > sha1_end_len("sha1_end_len");
	hls::stream< addr_t , 2 > sha1_digest("sha1_digest");
	hls::stream< bool , 2 > sha1_end_digest("sha1_end_digest");

	dedup_loop: while (!in.empty() || !end){
#pragma HLS PIPELINE
		//flush the buffers
		reset_buffers(sha1_msg, sha1_len, sha1_end_len, sha1_digest, sha1_end_digest);

		//move data to streams
		read_in(in, end, in_buffer, sha1_msg, sha1_len, sha1_end_len);

		//calculate sha1 hash
		xf::security::sha1< 32 >(sha1_msg, sha1_len, sha1_end_len, sha1_digest, sha1_end_digest);

		//check for duplicate
		bool sha1_end = sha1_end_digest.read();
		parse_buffer: while (!sha1_end){
			//create bus packet
			bram_packet to_bram;
			sc_packet tmp_packet = in_buffer.read();
#pragma HLS ARRAY_PARTITION variable=to_bram.data type=complete
			tmp_packet.hash = sha1_digest.read();
			convert_to_bram(tmp_packet, to_bram);

			//check for duplicate
			check_duplicate(to_bram, tmp_packet.is_duplicate);

			//pass to next stage
			write_out(tmp_packet, out);

			//check condition
			sha1_end = sha1_end_digest.read();
		}
	}
	return;
}
