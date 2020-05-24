#ifndef DEDUP_H
#define DEDUP_H

//general includes
#include <iostream>
#include <fstream>
//user includes
#include "bus_def.h"
#include "type_conversions.h"
//includes from vitis libraries
#include "hls_task.h"
#include "hls_stream.h"
#include "sha1.hpp"

//MACRO definitions
#define MSG_BUFF_SIZE 64 //size of the message FIFO to the SHA1 kernel

//function definitions

void dedup(bus_packet &in, ap_uint< 160 > &sha1_debug_out, bool &sha1_debug_end);

#endif //DEDUP_H
