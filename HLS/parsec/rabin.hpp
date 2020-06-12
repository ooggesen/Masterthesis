#ifndef _RABIN_H_
#define _RABIN_H_

#include "../bus_def.hpp"
#include "../parsec.hpp"

/* Define USED macro */
#define USED(x) { ulong y __attribute__ ((unused)); y = (ulong)x; }

enum {
  NWINDOW   = 32,
  MinSegment  = 1024,
  RabinMask = 0xfff,  // must be less than <= 0x7fff 
};

void rabininit(int winlen, unsigned rabintab[], unsigned rabinwintab[]);

void rabinseg_bc_packet(bc_packet &in, sc_packet &out, int winlen, unsigned rabintab[], unsigned rabinwintab[]);

#endif //_RABIN_H_

