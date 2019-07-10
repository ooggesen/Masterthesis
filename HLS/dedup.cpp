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
static void read_in(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< sc_packet > &meta_buffer,
		hls::stream< c_data_t > &data_buffer,
		hls::stream< ap_uint< 32 > , MSG_BUFF_SIZE > &sha1_msg,
		hls::stream< ap_uint< 64 > , 2 > &sha1_len,
		hls::stream< bool , 2 > &sha1_end_len)
{
	if (!meta_in.empty()){
		//write buffer
		sc_packet tmp_packet = meta_in.read();
		meta_buffer.write(tmp_packet);

		//write sha streams
		sha1_end_len.write(false);

		sha1_len.write((unsigned long long) tmp_packet.size);

		read_data: for (int i = 0; i < hls::ceil((double) tmp_packet.size.to_long()*8 / W_DATA); i++){
			int size_remaining = tmp_packet.size.to_long() - i*W_DATA/8;

			c_data_t current = data_in.read();
			//write to bram
			data_buffer.write(current);

			//write to sha1 kernel
			write_to_sha1_buff: for (int j = 0; j < W_DATA/32 ; j++){
				// stop after the last package in stream
				if (size_remaining - j*4 > 0){
					sha1_msg.write(current.range(31 + 32 * j, 32 * j));
				}
			}
		}
	}

	//write end of stream for sha1 function
	sha1_end_len.write(true);
}



static void convert_to_bram(hls::stream< sc_packet > &bram_meta_buffer,
		hls::stream< c_data_t > &bram_data_buffer,
		hls::stream< addr_t > &sha1_digest,
		hls::stream< sc_packet > &write_out_meta_buffer,
		hls::stream< c_data_t >  &write_out_data_buffer,
		bram_packet &to_bram){
	sc_packet meta = bram_meta_buffer.read();
	meta.hash = sha1_digest.read();
	write_out_meta_buffer.write(meta);

	to_bram.addr = meta.hash;

	copy_to_bram: for (int i = 0 ; i < hls::ceil((double)meta.size.to_long()*8  / W_DATA) ; i++){
		c_data_t packet = bram_data_buffer.read();
		write_out_data_buffer.write(packet);
		to_bram.data[i] = packet;
	}
}

static void check_duplicate(bram_packet to_bram,
		hls::stream< sc_packet > &dedup_meta_buffer,
		hls::stream< sc_packet > &write_out_data_buffer){
	bram_packet packet_r;
	packet_r.addr = to_bram.addr;

	sc_packet meta = dedup_meta_buffer.read();

	bram(false, true, to_bram, packet_r, meta.size);

	if (is_equal(to_bram.data, packet_r.data, meta.size)){
		meta.is_duplicate = true; //found duplicate
	} else {
		//found unique chunk
		bram(true, false, to_bram, packet_r, meta.size); //write to bram
		meta.is_duplicate = false;
	}

	write_out_data_buffer.write(meta);
}


static void write_out(hls::stream< sc_packet > &write_out_meta_buffer,
		hls::stream< c_data_t > &write_out_data_buffer,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out){
		sc_packet tmp_meta = write_out_meta_buffer.read();
		meta_out.write(tmp_meta);

		for (c_size_t i = 0 ; i < hls::ceil((double) tmp_meta.size*8/W_DATA) ; i++){
			data_out.write(write_out_data_buffer.read());
		}
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
void dedup(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		bool end,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out){
	//buffer
	hls::stream< sc_packet , 2> bram_meta_buffer("bram_meta_buffer");
	hls::stream< sc_packet, 2> dedup_meta_buffer("dedup_meta_buffer");
	hls::stream< sc_packet , 2> write_out_meta_buffer("write_out_meta_buffer");
	hls::stream< c_data_t, SC_STREAM_SIZE > bram_data_buffer("bram_data_buffer");
	hls::stream< c_data_t, SC_STREAM_SIZE > write_out_data_buffer("wrie_out_data_buffer");
	//sha1 streams
	hls::stream< ap_uint< 32 > , MSG_BUFF_SIZE > sha1_msg("sha1_msg");
	hls::stream< ap_uint< 64 > , 2 > sha1_len("sha1_len");
	hls::stream< bool , 2 > sha1_end_len("sha1_end_len");
	hls::stream< addr_t , 2 > sha1_digest("sha1_digest");
	hls::stream< bool , 2 > sha1_end_digest("sha1_end_digest");

	dedup_loop: while (!meta_in.empty() || !end){
		//flush the buffers
		//reset_buffers(sha1_msg, sha1_len, sha1_end_len, sha1_digest, sha1_end_digest);

		//move data to streams
		read_in(meta_in, data_in, bram_meta_buffer, bram_data_buffer, sha1_msg, sha1_len, sha1_end_len);

		//calculate sha1 hash
		xf::security::sha1< 32 >(sha1_msg, sha1_len, sha1_end_len, sha1_digest, sha1_end_digest);

		flush_buffer<bool, 2>(sha1_end_digest);
		//check for duplicate
		//create bus packet
		bram_packet to_bram;
		convert_to_bram(bram_meta_buffer, bram_data_buffer, sha1_digest, dedup_meta_buffer, write_out_data_buffer, to_bram);

		//check for duplicate
		check_duplicate(to_bram, dedup_meta_buffer, write_out_meta_buffer);

		//pass to next stage
		write_out(write_out_meta_buffer, write_out_data_buffer, meta_out, data_out);
	}
}
