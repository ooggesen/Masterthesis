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

void fpwinreduce(unsigned irr, int winlen, unsigned x, unsigned rabintab[], unsigned &winval) {
    int i;

    winval = 0;
    winval = ((winval << 8) | x) ^ rabintab[(winval >> 24)];
    for (i = 1; i < winlen; i++)
        winval = (winval << 8) ^ rabintab[(winval >> 24)];
}

void fpmkwinredtab(unsigned irr, int winlen, unsigned rabintab[], unsigned rabinwintab[]) {
    int i;

    for (i = 0; i < 256; i++)
        fpwinreduce(irr, winlen, i, rabintab, rabinwintab[i]);
    return;
}

void rabininit(int winlen, unsigned rabintab[], unsigned rabinwintab[]) {
    //rabintab = malloc(256*sizeof rabintab[0]);
    //rabinwintab = malloc(256*sizeof rabintab[0]);
	unsigned irrpoly = 0x45c2b6a1;
    fpmkredtab(irrpoly, 0, rabintab);
    fpmkwinredtab(irrpoly, winlen, rabintab, rabinwintab);
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
	out.size = len;
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
	in.size = in.size - len;
	byte_pos_bc = len % (W_DATA_BIG_CHUNK/8);
	array_pos_bc = (int) len / (W_DATA_BIG_CHUNK/8);
	for (int i = 0 ; i < BC_ARRAY_SIZE ; i++){
		for (int b = 0 ; b < W_DATA_BIG_CHUNK / 8 ; b++){
			if (array_pos_bc * W_DATA_BIG_CHUNK + byte_pos_bc * 8 > in.size * 8){
				//fill with 0
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

/*
 * Calculates the rabin fingerprint and segements the data accordingly
 * -> does not set the l1 and l2 positions of the small chunk output
 *
 * @param in    : big chunk data packet, segmented data is removed from data attribute and size infos are updated accordingly
 * @param out   : small chunk data packet segmented with byte precision from the big chunk data
 * @param winlen:
 */
void rabinseg_bc_packet(bc_packet &in, sc_packet &out, int winlen, unsigned rabintab[], unsigned rabinwintab[]) {
    int i;
    unsigned h;
    unsigned x;

    USED(winlen); //TODO what does this do?
    //if size small than rabin window return the whole big chunk packet as small chunk packet
    if (in.size < NWINDOW){
    	segment_bc_packet(in, in.size.to_long(), out);
    	return;
    }

    h = 0;
    for (i = 0; i < NWINDOW; i++) {
#pragma HLS UNROLL
        x = h >> 24;

        unsigned char byte = in.data[0].range(7 + 8*i, 8*i);
        h = (h << 8) | byte;

        h ^= rabintab[x]; //TODO understand the rabintab
    }

    if ((h & RabinMask) == 0){
        segment_bc_packet(in, i, out);
        return;
    }

    int pos;
    while (i < in.size) {
    	pos = ((i - NWINDOW)*8) % W_DATA_BIG_CHUNK;
        x = in.data[(int) (i - NWINDOW)*8 / W_DATA_BIG_CHUNK].range(7 + pos, pos);

        h ^= rabinwintab[x]; //TODO same as above
        x = h >> 24;
        h <<= 8;

        pos = (i * 8) % W_DATA_BIG_CHUNK;
        h |= in.data[(int) i*8 / W_DATA_BIG_CHUNK].range(7 + pos, pos);
        i++;

        h ^= rabintab[x]; //TODO same as above
        if ((h & RabinMask) == 0){
        	segment_bc_packet(in, i, out);
        	return;
        }
    }
    segment_bc_packet(in, in.size.to_long() , out);
}

