/**
 * @file rabin.cpp
 *
 * @brief Rabin fingerprint algorithm and helper functions.
 *
 * This rabin fingerprint algorithm is takes from the dedup kernel of the PARSEC benchmark suite.
 * It is adapted to the needs of thesis.
 *
 * https://github.com/bamos/parsec-benchmark/tree/master
 * path:
 * 	pkgs/kernels/dedup/src/rabin.c
 *
 * This algorithm added a maximum chunk size to the algorithm.
 *
 * TODOs:
 * 	implement a minimum chunk size
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "rabin.hpp"


static void fpreduce(unsigned x, unsigned irr, unsigned &out) {
    int i;

    for (i = 32; i != 0; i--) {
#pragma HLS LOOP_FLATTEN
        if (x >> 31) {
            x <<= 1;
            x ^= irr;
        } else
            x <<= 1;
    }
    out = x;
}

static void fpmkredtab(unsigned irr, unsigned tab[]) {
#pragma HLS INLINE recursive
    int i;

    for (i = 0; i < 256; i++){
#pragma HLS LOOP_FLATTEN
        fpreduce(i , irr, tab[i]);
    }
}

static void fpwinreduce(unsigned x, unsigned rabintab[], unsigned &winval) {
	winval = x ^ rabintab[0];
}

static void fpmkwinredtab(unsigned rabintab[], unsigned rabinwintab[]) {
#pragma HLS INLINE recursive
    int i;

    for (i = 0; i < 256; i++){
#pragma HLS LOOP_FLATTEN
        fpwinreduce(i, rabintab, rabinwintab[i]);
    }
}

void rabininit(unsigned rabintab[], unsigned rabinwintab[]) {
    //rabintab = malloc(256*sizeof rabintab[0]);
    //rabinwintab = malloc(256*sizeof rabintab[0]);
	unsigned irrpoly = 0x45c2b6a1;
    fpmkredtab(irrpoly, rabintab);
    fpmkwinredtab(rabintab, rabinwintab);
}



void rabinseg_in_stream(hls::stream< ap_uint< 8 > > &in,
		c_size_t &bc_size,
		hls::stream< ap_uint< 8 > > &out,
		c_size_t &size,
		unsigned rabintab[],
		unsigned rabinwintab[]){
	hls::stream< ap_uint< 8 > , NWINDOW> buffer;
	c_size_t size_buffer = 0;
	c_size_t bc_size_buffer = bc_size;

	int i;
	unsigned h = 0;
	unsigned x;

	init_buffer: for (i = 0; i < NWINDOW; i++) {
#pragma HLS PIPELINE II=1
		if (bc_size_buffer == 0) //end of big chunk
			break;

		x = h >> 24;

		ap_uint< 8 > byte = in.read();
		size_buffer++;
		bc_size_buffer--;

		h = (h << 8) | byte;
		out.write(byte);
		buffer.write(byte);


		h ^= rabintab[x];
	}

    segment_stream: for ( ; i < MAX_SMALL_CHUNK_SIZE/8 ; i++ ) {
#pragma HLS PIPELINE II=2
    	if ((h & RabinMask) == 0 || bc_size_buffer == 0)
    		break;

        x = buffer.read();
        h ^= rabinwintab[x];

        ap_uint< 8 > byte = in.read();
        out.write(byte);
        buffer.write(byte);

        size_buffer++;
        bc_size_buffer--;

        x = h >> 24;
        h = byte | (h << 8);

        h ^= rabintab[x];
    }

	//flush the buffer
	flush_buffer: for (i = 0 ; i < NWINDOW ; i++){
		if (buffer.empty())
			break;
		buffer.read();
	}

	//write out
	size = size_buffer;
	bc_size = bc_size_buffer;
}

