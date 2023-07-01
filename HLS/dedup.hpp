#ifndef DEDUP_HPP
#define DEDUP_HPP

//user includes
#include "bus_def.hpp"
#include "bram.hpp"
//includes from vitis libraries
#include "hls_stream.h"
#include "sha1.hpp"
#include "hls_math.h"

//MACRO definitions
#define MSG_BUFF_SIZE 64 //size of the message FIFO to the SHA1 kernel

//function definitions
void dedup(hls::stream< sc_packet > &meta_in,
		hls::stream< c_data_t > &data_in,
		bool end,
		hls::stream< sc_packet > &meta_out,
		hls::stream< c_data_t > &data_out);

#endif //DEDUP_HPP
