/*
 * This file contains the helper functions for manipulation of the bus data types
 */

#include "bus_def.h"

bool is_equal(sc_data_t a[SC_ARRAY_SIZE], sc_data_t b[SC_ARRAY_SIZE]){
	for (int i = 0 ; i < SC_ARRAY_SIZE ; i++ ){
#pragma HLS UNROLL
		if (a[i] != b[i]) return false;
	}

	return true;
}
