/*
 * This file contains the fragment refine function which takes a big chunk and cuts it into smaller chunks
 */

#include "fragment_refine.hpp"



/*
 * read in pipeline stage
 */
void read_in(hls::stream< bc_packet > &in, bc_packet &buffer){
	buffer = bc_packet(in.read());
}

void set_arguments(bc_packet bcp,  sc_packet &out, unsigned long long &l2_pos){
	out.l2_pos = l2_pos++;
	out.l1_pos = bcp.l1_pos;
	out.last_l2_chunk = bcp.size.to_long() == 0;
	out.hash = 0; //default value, is correctly set by the dedup kernel
	out.is_duplicate = false; //default value, is correctly set by the dedup kernel
}



/*
 * kernel which segments the big chunks into small chunks
 *
 * @param in : input stream of big chunk data
 * @param out: output stream of small chunks
 */
void fragment_refine(hls::stream< bc_packet > &in, bool end, hls::stream< sc_packet > &out){
	//intialize the rabin lookup tables
	int winlen = 0;
	unsigned rabintab[256], rabinwintab[256];
	rabininit(winlen, rabintab, rabinwintab);

	//buffer for processed big chunk
	bc_packet buffer;
#pragma HLS ARRAY_PARTITION variable=buffer type=complete

	do{
		//read data in
		read_in(in, buffer);

		//segment the big chunk packet into a stream of small chunk packets
		unsigned long long l2_pos = 0;
		refine_loop: while(buffer.size.to_long() > 0){
			sc_packet packet;
#pragma HLS ARRAY_PARTITION variable=packet type=complete
			//segment a small chunk
			rabinseg_bc_packet(buffer, packet, winlen, rabintab, rabinwintab);

			//write its arguments
			set_arguments(buffer, packet, l2_pos);

			out.write(packet);
		}
	} while(!end || !in.empty());
}
