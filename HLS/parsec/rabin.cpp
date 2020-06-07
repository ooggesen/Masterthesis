#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include "rabin.h"

/* Functions to compute rabin fingerprints */

/*
uint32_t bswap32(x)uint32_t x; {
    return ((x << 24) & 0xff000000) |
           ((x << 8) & 0x00ff0000) |
           ((x >> 8) & 0x0000ff00) |
           ((x >> 24) & 0x000000ff);
}
*/

void fpreduce(ap_uint< 32 > x, ap_uint< 32 > irr, ap_uint< 32 > &out) {
    int i;

    for (i = 32; i != 0; i--) {
        if (out >> 31) {
            out <<= 1;
            out ^= irr;
        } else
            out <<= 1;
    }
}

static void fpmkredtab(ap_uint< 32 > irr, int s, ap_uint< 32 > tab[]) {
    ap_uint< 32 > i;

    for (i = 0; i < 256; i++)
        fpreduce(i << s, irr, tab[i.to_int()]);
    return;
}

void fpwinreduce(ap_uint< 32 > irr, int winlen, ap_uint< 32 > x, ap_uint< 32 > rabintab[], ap_uint< 32 > &winval) {
    int i;

    winval = 0;
    winval = ((winval << 8) | x) ^ rabintab[(winval >> 24).to_int()];
    for (i = 1; i < winlen; i++)
        winval = (winval << 8) ^ rabintab[winval >> 24];
}

static void fpmkwinredtab(ap_uint< 32 > irr, int winlen, ap_uint< 32 > rabintab[], ap_uint< 32 > rabinwintab[]) {
    ap_uint< 32 > i;

    for (i = 0; i < 256; i++)
        fpwinreduce(irr, winlen, i, rabintab, rabinwintab[i]);
    return;
}

void rabininit(int winlen, ap_uint< 32 > rabintab[], ap_uint< 32 > rabinwintab[]) {
    //rabintab = malloc(256*sizeof rabintab[0]);
    //rabinwintab = malloc(256*sizeof rabintab[0]);
	ap_uint< 32 > irrpoly = 0x45c2b6a1;
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
void segment_bc_packet(bc_packet &in, int len, sc_packet &out){
	//write out segmented small chunk to param out
	out.size = len*8;
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
			if (array_pos_bc * W_DATA_BIG_CHUNK + byte_pos_bc * 8 > in.size){
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
 */
void rabinseg_bc_packet(bc_packet &in, sc_packet &out, int winlen, ap_uint< 32 > rabintab[], ap_uint< 32 > rabinwintab[]) {
    int i;
    ap_uint< 32 > h;
    ap_uint< 32 > x;

    USED(winlen); //TODO what does this do?
    //if size small than rabin window return the whole big chunk packet as small chunk packet
    if (in.size < NWINDOW * 8){
    	segment_bc_packet(in, (int) in.size.to_int()/8, out);
    	return;
    }

    h = 0;
    for (i = 0; i < NWINDOW; i++) {
        x = h >> 24;
        h = (h << 8) | in.data[0].range(7 + 8*i, 8*i);
        h ^= rabintab[x]; //TODO understand the rabintab
    }

    if ((h & RabinMask) == 0){
        segment_bc_packet(in, i, out);
        return;
    }

    int pos;
    while (i*8 < in.size) {
    	pos = ((i - NWINDOW)*8) % W_DATA_BIG_CHUNK;
        x = in.data[(int) i*8 / W_DATA_BIG_CHUNK].range(7 + pos, pos);

        h ^= rabinwintab[x]; //TODO same as above
        x = h >> 24;
        h <<= 8;

        pos = (i * 8) % W_DATA_BIG_CHUNK;
        h |= in.data[(int) i*8 / W_DATA_BIG_CHUNK].range(7 + pos, pos);;
        i++;

        h ^= rabintab[x]; //TODO same as above
        if ((h & RabinMask) == 0){
        	segment_bc_packet(in, i, out);
        	return;
        }
    }
    segment_bc_packet(in, in.size, out);
}

