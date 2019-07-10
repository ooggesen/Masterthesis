#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include "rabin.hpp"


static void fpreduce(unsigned x, unsigned irr, unsigned &out) {
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

static void fpwinreduce(unsigned x, unsigned rabintab[], unsigned &winval) {
    int i;

    winval = 0;
    winval = ((winval << 8) | x) ^ rabintab[(winval >> 24)];
}

static void fpmkwinredtab(unsigned rabintab[], unsigned rabinwintab[]) {
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
 * Segments a unsigned byte FIFO into small grained chunks
 *
 * @param in : Input FIFO which contains continuous data to be segmented
 * @param end: signals end of process -> if FIFO in is empty and end flag is set the process returns
 * @param out: empty FIFO which will hold exactly the data for one coarse grained chunk
 */
void rabinseg_in_stream(hls::stream< ap_uint< 8 > > &in,
		bool end,
		hls::stream< ap_uint< 8 > > &out,
		c_size_t &size,
		unsigned rabintab[],
		unsigned rabinwintab[]){
	hls::stream< ap_uint< 8 > , NWINDOW> buffer;
	c_size_t size_buffer = 0;

	int i = NWINDOW;
	unsigned h = 0;
	unsigned x;

	init_hash: for (int i = 0; i < NWINDOW; i++) {
		x = h >> 24;

		if (end && in.empty())
			goto write_out_size;
		ap_uint< 8 > byte = in.read();
		size_buffer++;

		h = (h << 8) | byte;
		out.write(byte);
		buffer.write(byte);


		h ^= rabintab[x];
	}

    segment_stream: for (int i = 0 ; i < MAX_SMALL_CHUNK_SIZE/8 ; i++ ) {
    	if ((h & RabinMask) == 0)
    		goto write_out_size;

        x = buffer.read();
        h ^= rabinwintab[x];

    	if (end && in.empty())
    		goto write_out_size;
        ap_uint< 8 > byte = in.read();
        size_buffer++;

        x = h >> 24;
        h = byte | (h << 8);

        out.write(byte);
        buffer.write(byte);

        h ^= rabintab[x];
    }

    write_out_size:
	while(!buffer.empty())
		buffer.read();
	size = size_buffer;
}

