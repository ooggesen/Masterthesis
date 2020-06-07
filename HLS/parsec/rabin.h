#ifndef _RABIN_H_
#define _RABIN_H_

#include "../bus_def.h"
#include "../parsec.h"

/* Define USED macro */
#define USED(x) { ulong y __attribute__ ((unused)); y = (ulong)x; }

enum {
  NWINDOW   = 32,
  MinSegment  = 1024,
  RabinMask = 0xfff,  // must be less than <= 0x7fff 
};

void rabininit(int, ap_uint< 32 >, ap_uint< 32 >);

void rabinseg_bc_packet(bc_packet &in, sc_packet &out, int winlen, ap_uint< 32 > rabintab[], ap_uint< 32 > rabinwintab[]);

#endif //_RABIN_H_

