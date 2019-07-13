#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

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

	int i = NWINDOW;
	unsigned h = 0;
	unsigned x;

	init_hash: for (int i = 0; i < NWINDOW; i++) {
		x = h >> 24;

		if (bc_size_buffer == 0)
			goto write_out_size;
		ap_uint< 8 > byte = in.read();
		size_buffer++;
		bc_size_buffer--;

		h = (h << 8) | byte;
		out.write(byte);
		buffer.write(byte);


		h ^= rabintab[x];
	}

    segment_stream: for (int i = 0 ; i < MAX_SMALL_CHUNK_SIZE/8 ; i++ ) {
#pragma HLS PIPELINE II=2
    	if ((h & RabinMask) == 0)
    		goto write_out_size;

        x = buffer.read();
        h ^= rabinwintab[x];

    	if (bc_size_buffer == 0)
    		goto write_out_size;
        ap_uint< 8 > byte = in.read();
        out.write(byte);
        buffer.write(byte);

        size_buffer++;
        bc_size_buffer--;

        x = h >> 24;
        h = byte | (h << 8);

        h ^= rabintab[x];
    }

    write_out_size:
	//flush the buffer
	for (int i = 0 ; i < NWINDOW ; i++){
		if (buffer.empty())
			break;
		buffer.read();
	}
	size = size_buffer;
	bc_size = bc_size_buffer;
}

