/*
 * This file contains the fragment refine function which takes a big chunk and cuts it into smaller chunks
 */

#include "fragment_refine.hpp"



/*
 * read in pipeline stage
 */
void read_in(hls::stream< bc_packet > &meta_in, hls::stream< c_data_t > &data_in, bc_packet &meta_out, hls::stream< ap_uint< 8 > > &data_out){
	if (!meta_in.empty()){
		bc_packet packet = meta_in.read();
		meta_out = bc_packet(packet);

		read_bc_data: for (int i = 0 ; i < hls::ceil((double) packet.size.to_long()*8 / W_DATA) || i < BC_STREAM_SIZE ; i++){
			c_data_t data_in_buffer = data_in.read();
			for (int j = 0 ; j < W_DATA/8 ; j++){
				if (packet.size.to_long() > i*W_DATA/8 + j){
					data_out.write(data_in_buffer.range(7 + 8*j, 8*j));
				}
			}
		}
	}
}



void convert_to_sc(bc_packet &meta_in,
		hls::stream< ap_uint< 8 > > &data_in,
		unsigned &l2_pos,
		c_size_t sc_size,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out){
	c_data_t c_data_buffer = 0;
	convert_loop: for (int i = 0 ; i < hls::ceil((double)sc_size.to_long()*8 / W_DATA) || i < SC_STREAM_SIZE ; i++){
		for (int j = 0 ; j < W_DATA/8 ; j++){
#pragma HLS UNROLL
			if (i*W_DATA/8 + j < sc_size.to_long()){
				//read data
				c_data_buffer.range(7 + j*8, j*8) = data_in.read();
			} else {
				//fill with zeros
				c_data_buffer.range(7 + j*8, j*8) = 0;
			}
		}
		data_out.write(c_data_buffer);
	}

	meta_in.size -= sc_size;

	sc_packet scp;
	scp.size = sc_size;
	scp.l2_pos = l2_pos++;
	scp.l1_pos = meta_in.l1_pos;
	scp.last_l2_chunk = meta_in.size.to_long() == 0;
	scp.hash = 0; //default value, is correctly set by the dedup kernel
	scp.is_duplicate = false; //default value, is correctly set by the dedup kernel

	meta_out.write(scp);
}



/*
 * kernel which segments the big chunks into small chunks
 *
 * @param in : input stream of big chunk data
 * @param out: output stream of small chunks
 */
void fragment_refine(hls::stream< bc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		bool end,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out){
	//intialize the rabin lookup tables
	unsigned rabintab[256], rabinwintab[256];
	rabininit(rabintab, rabinwintab);

	//buffer for processed big chunk
	bc_packet meta_buffer;
	hls::stream< ap_uint< 8 > , BC_STREAM_SIZE*W_DATA/8 > bc_data_buffer("bc_data_buffer");
	hls::stream< ap_uint< 8 > , SC_STREAM_SIZE*W_DATA/8 > sc_data_buffer("sc_data_buffer");

	while(!end || !meta_in.empty()){
#pragma HLS PIPELINE
		//read data in
		read_in(meta_in, data_in, meta_buffer, bc_data_buffer);

		//segment the big chunk packet into a stream of small chunk packets
		unsigned l2_pos = 0;
		refine_loop: while(meta_buffer.size.to_long() > 0){
			//segment a small chunk
			c_size_t sc_size;
			rabinseg_in_stream(bc_data_buffer, end,
					sc_data_buffer, sc_size, rabintab, rabinwintab);

			//write out as sc_packet type
			convert_to_sc(meta_buffer, sc_data_buffer, l2_pos, sc_size, meta_out, data_out);
		}
	}
}
