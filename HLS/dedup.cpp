/*
 * @file dedup.cpp
 *
 * @brief Contains the dedup pipeline stage
 *
 * This module checks if input chunks are a duplicate in the data stream and accesses a BRAM module for storage of all unique chunks it encountered so far.
 * Can only be instantiated once, since access to BRAM can not be shared in current implementation.
 *
 * @author Ole Oggesen
 * @bugs No known bugs.
 */


#include "dedup.hpp"



static void read_in(
		hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out,
		hls::stream< ap_uint< 32 > > &sha1_msg,
		hls::stream< ap_uint< 64 > > &sha1_len,
		hls::stream< bool , 2 > &sha1_end_len)
{
	//write buffer
	sc_packet meta = meta_in.read();
	meta_out.write(meta);

	//write sha streams
	sha1_end_len.write(false);

	sha1_len.write((unsigned long long) meta.size);

	read_data: for (c_size_t i = 0; i < (int) MAX_SMALL_CHUNK_SIZE/W_DATA + 1; i++){
#pragma HLS LOOP_FLATTEN off
		long size_remaining = meta.size.to_long() - i.to_long()*W_DATA/8;
		if (size_remaining <= 0)
			break;

		c_data_t current = data_in.read();
		//write data to bram
		data_out.write(current);

		//write to sha1 kernel
		write_to_sha1_buff: for (int j = 0; j < W_DATA/32 ; j++){
	#pragma HLS PIPELINE II=1
			// stop after the last package in stream
			if (size_remaining - j*4 > 0){
				sha1_msg.write(current.range(31 + 32 * j, 32 * j));
			}
		}
	}


	//write end of stream for sha1 function
	sha1_end_len.write(true);
}



static void check_duplicate(
		hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< addr_t > &sha1_digest,
		hls::stream< bool > &sha1_end_digest,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out){
	bool end = sha1_end_digest.read();

	//loop only executed once
	while(!end){
#pragma HLS LOOP_TRIPCOUNT min=1 max=1 avg=1

		//empty packet to contain read data
		addr_t hash = sha1_digest.read();

		sc_packet meta = meta_in.read();
		meta.hash = hash;

		//prepare data to write and compare in bram
		bram_packet packet_r, packet_w;
		packet_r.addr = hash;
		packet_w.addr = hash;

		copy_to_bram: for (int i = 0 ; i < SC_STREAM_SIZE ; i++){
#pragma HLS PIPELINE II=6
			if (i >= hls::ceil((double)meta.size.to_long()*8  / W_DATA))
				break;

			c_data_t packet = data_in.read();
			data_out.write(packet);
			packet_w.data[i] = packet;
		}

		bram(false, true, packet_w, packet_r, meta.size);

		if (is_equal(packet_w.data, packet_r.data, meta.size)){
			meta.is_duplicate = true; //found duplicate
		} else {
			//found unique chunk
			bram(true, false, packet_w, packet_r, meta.size); //write to bram
			meta.is_duplicate = false;
		}

		meta_out.write(meta);

		end = sha1_end_digest.read();
	}
}


static void write_out(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool > &end_in,
		bool &end,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out){
	//write data
	sc_packet meta = meta_in.read();
	meta_out.write(meta);

	write: for (c_size_t i = 0 ; i < MAX_SMALL_CHUNK_SIZE / W_DATA ; i++){
#pragma HLS PIPELINE II=6
		if (i >= hls::ceil((double) meta.size*8/W_DATA))
			break;
		data_out.write(data_in.read());
	}
	end_out.write(false);

	//set the end flg for the while loop
	end = end_in.read();
}



void dedup(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool > &end_in,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out){
	//buffer
	hls::stream< sc_packet , 2> bram_meta("bram_meta");
	hls::stream< sc_packet, 2> dedup_meta("dedup_meta");
	hls::stream< sc_packet , 2> write_out_meta("write_out_meta");
	hls::stream< c_data_t, SC_STREAM_SIZE > bram_data("bram_data");
	hls::stream< c_data_t, SC_STREAM_SIZE > write_out_data("wrie_out_data");
	hls::stream< bool , 2 > write_out_end("write_out_end");
	//sha1 streams
	hls::stream< ap_uint< 32 > , MSG_BUFF_SIZE > sha1_msg("sha1_msg");
	hls::stream< ap_uint< 64 > , 2 > sha1_len("sha1_len");
	hls::stream< bool , 2 > sha1_end_len("sha1_end_len");
	hls::stream< addr_t , 2 > sha1_digest("sha1_digest");
	hls::stream< bool , 2 > sha1_end_digest("sha1_end_digest");

	bool end = end_in.read();

	dedup_loop: while (!end){
#pragma HLS LOOP_TRIPCOUNT min=1 max=1 avg=1
		read_in(meta_in, data_in, bram_meta, bram_data, sha1_msg, sha1_len, sha1_end_len);

		//calculate sha1 hash
		xf::security::sha1< 32 >(sha1_msg, sha1_len, sha1_end_len, sha1_digest, sha1_end_digest);

		//check for duplicate
		check_duplicate(bram_meta, bram_data, sha1_digest, sha1_end_digest, write_out_meta, write_out_data);

		//pass to next stage and update end flag
		write_out(write_out_meta, write_out_data, end_in, end, meta_out, data_out, end_out);
	}

	end_out.write(true);
}
