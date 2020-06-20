#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include "rabin.hpp"

/* Functions to compute rabin fingerprints */

/*
uint32_t bswap32(x)uint32_t x; {
    return ((x << 24) & 0xff000000) |
           ((x << 8) & 0x00ff0000) |
           ((x >> 8) & 0x0000ff00) |
           ((x >> 24) & 0x000000ff);
}
*/

void fpreduce(unsigned x, unsigned irr, unsigned &out) {
    int i;


    for (i = 32; i != 0; i--) {
        if (x >> 31) {
            x <<= 1;
            x ^= irr;
        } else
            x <<= 1;
    }
    out = x;
}

static void fpmkredtab(unsigned irr, int s, unsigned tab[]) {
    int i;

    for (i = 0; i < 256; i++)
        fpreduce(i << s, irr, tab[i]);
    return;
}

void fpwinreduce(unsigned x, unsigned rabintab[], unsigned &winval) {
    int i;

    winval = 0;
    winval = ((winval << 8) | x) ^ rabintab[(winval >> 24)];
}

void fpmkwinredtab(unsigned rabintab[], unsigned rabinwintab[]) {
    int i;

    for (i = 0; i < 256; i++)
        fpwinreduce(i, rabintab, rabinwintab[i]);
}

void rabininit(unsigned rabintab[], unsigned rabinwintab[]) {
    //rabintab = malloc(256*sizeof rabintab[0]);
    //rabinwintab = malloc(256*sizeof rabintab[0]);
	unsigned irrpoly = 0x45c2b6a1;
    fpmkredtab(irrpoly, 0, rabintab);
    fpmkwinredtab(rabintab, rabinwintab);
}


/*
 * Segments a big chunk bus packet with byte precision into a small chunk bus packet
 *
 * @param in  : big chunk packet to be segmented
 * @param len : position of segmentation in byte
 * @param out : small chunk packet containing the segmented data
 */
void segment_bc_packet(bc_packet &in, unsigned long len, sc_packet &out){
	//write out segmented small chunk to param out
	out.size = (unsigned long long) len;
	int byte_pos_bc = 0, array_pos_bc = 0;
	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++){
		for (int b = 0 ; b < W_DATA_SMALL_CHUNK / 8 ; b++){
			if (array_pos_bc * W_DATA_BIG_CHUNK + byte_pos_bc * 8 > len * 8){
				//fill with zeros
				out.data[i].range(7 + 8 * b, 8 * b) = 0;
			} else {
				//copy data byte aligned
				out.data[i].range(7 + 8 * b, 8 * b) = in.data[array_pos_bc].range(7 + 8 * byte_pos_bc , 8 * byte_pos_bc);
			}

			//update big chunk positional arguments
			byte_pos_bc++;
			if (byte_pos_bc * 8 >= W_DATA_BIG_CHUNK){
				array_pos_bc++;
				byte_pos_bc = 0;
			}
		}
	}

	//update left over big chunk data in param in
	in.size = (unsigned long long) in.size.to_long() - len;
	byte_pos_bc = len % (W_DATA_BIG_CHUNK/8);
	array_pos_bc = (int) len / (W_DATA_BIG_CHUNK/8);
	for (int i = 0 ; i < BC_ARRAY_SIZE ; i++){
		for (int b = 0 ; b < W_DATA_BIG_CHUNK / 8 ; b++){
			if (array_pos_bc >= BC_ARRAY_SIZE){
				//fill with 0 if end of file is reached
				in.data[i].range(7 + 8 * b, 8 * b) = 0;
			} else {
				//move left over data to beginning of array
				in.data[i].range(7 + 8 * b, 8 * b) = in.data[array_pos_bc].range(7 + 8 * byte_pos_bc, 8 * byte_pos_bc);
			}

			//update big chunk positional arguments
			byte_pos_bc++;
			if (byte_pos_bc * 8 >= W_DATA_BIG_CHUNK){
				array_pos_bc++;
				byte_pos_bc = 0;
			}
		}
	}
}


void read_in(bc_packet &in, bc_packet &buffer){
	buffer = bc_packet(in);
}



void init_hash(unsigned &h, unsigned &x, bc_packet &bcp, unsigned rabintab[]){
	init_hash_bc: for (int i = 0; i < NWINDOW; i++) {
#pragma HLS UNROLL
		x = h >> 24;

		unsigned char byte = bcp.data[0].range(7 + 8*i, 8*i);
		h = (h << 8) | byte;

		h ^= rabintab[x]; //TODO understand the rabintab
	}
}



void write_out(bc_packet &bc_buffer, sc_packet &sc_buffer, bc_packet &bc_out, sc_packet &sc_out){
	bc_out = bc_packet(bc_buffer);
	sc_out = sc_packet(sc_buffer);
}



/*
 * Calculates the rabin fingerprint and segements the data accordingly
 * -> does not set the l1 and l2 positions of the small chunk output
 *
 * @param in    : big chunk data packet, segmented data is removed from data attribute and size infos are updated accordingly
 * @param out   : small chunk data packet segmented with byte precision from the big chunk data
 */
void rabinseg_bc_packet(bc_packet &in, sc_packet &out, unsigned rabintab[], unsigned rabinwintab[]) {
	bc_packet in_buffer;
	sc_packet out_buffer;
#pragma HLS ARRAY_PARTITION variable=in_buffer type=complete
#pragma HLS ARRAY_PARTITION variable=out_buffer type=complete
	read_in(in, in_buffer);

	//helper variables
    int i = NWINDOW;
    unsigned h = 0;
    unsigned x;

    //if size small than rabin window return the whole big chunk packet as small chunk packet
    if (in_buffer.size.to_long() < NWINDOW){
    	segment_bc_packet(in_buffer, in_buffer.size.to_long(), out_buffer);
    	goto write_out_tag;
    }

    init_hash(h, x, in_buffer, rabintab);

    if ((h & RabinMask) == 0){
        segment_bc_packet(in_buffer, i, out_buffer);
        goto write_out_tag;
    }

    int pos;
    segment_bc: while (i < in.size.to_long()) {
    	pos = ((i - NWINDOW)*8) % W_DATA_BIG_CHUNK;
        x = in_buffer.data[(int) (i - NWINDOW)*8 / W_DATA_BIG_CHUNK].range(7 + pos, pos);

        h ^= rabinwintab[x];
        x = h >> 24;
        h <<= 8;

        pos = (i * 8) % W_DATA_BIG_CHUNK;
        h |= in_buffer.data[(int) i*8 / W_DATA_BIG_CHUNK].range(7 + pos, pos);
        i++;

        h ^= rabintab[x];
        if ((h & RabinMask) == 0){
        	segment_bc_packet(in_buffer, i, out_buffer);
        	goto write_out_tag;
        }
    }
    segment_bc_packet(in_buffer, in.size.to_long() , out_buffer);

    //write buffer to output
    write_out_tag:
	write_out(in_buffer, out_buffer, in, out);
}



/*
 * Segments a unisigned byte FIFO into coarse grained chunks
 *
 * @param in : Input FIFO which contains continous data to be segmented
 * @param end: signals end of process -> if FIFO in is empty and end flag is set the process returns
 * @param out: empty FIFO which will hold exactly the data for one coarse grained chunk
 */
void rabinseg_in_stream(hls::stream< ap_uint< 8 > > &in, bool end, hls::stream< ap_uint< 8 > > &out, unsigned rabintab[], unsigned rabinwintab[]){
	hls::stream< ap_uint< 8 > > buffer;

	int i = NWINDOW;
	unsigned h = 0;
	unsigned x;

	init_hash_from_stream: for (int i = 0; i < NWINDOW; i++) {
#pragma HLS UNROLL
		if (end && in.empty())
			return;

		x = h >> 24;

		unsigned char byte = in.read();
		h = (h << 8) | byte;
		out.write(byte);
		buffer.write(byte);


		h ^= rabintab[x];
	}

	if ((h & RabinMask) == 0)
		return;

    segment_stream: while (!in.empty() || !end) {
    	if (end && in.empty())
    		return;

        x = buffer.read();

        h ^= rabinwintab[x];
        x = h >> 24;
        h <<= 8;

        unsigned char byte = in.read();
        h |= byte;
        out.write(byte);
        buffer.write(byte);

        h ^= rabintab[x];
        if ((h & RabinMask) == 0){
        	return;
        }
    }
}

