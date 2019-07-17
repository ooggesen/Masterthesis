#ifndef DEDUP_HPP
#define DEDUP_HPP

//user includes
#include "bus_def.hpp"
#include "bram.hpp"
//includes from vitis libraries
#include "hls_stream.h"
#include "sha1.hpp"
#include "hls_math.h"

#define MSG_BUFF_SIZE ((int)MAX_SMALL_CHUNK_SIZE/32)

//function definitions
void dedup(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool > &end_in,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out);

#endif //DEDUP_HPP
