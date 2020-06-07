#ifndef DEDUP_H
#define DEDUP_H

//user includes
#include "bus_def.h"
#include "bram.h"
//includes from vitis libraries
#include "hls_task.h"
#include "hls_stream.h"
#include "sha1.hpp"

//MACRO definitions
#define MSG_BUFF_SIZE 64 //size of the message FIFO to the SHA1 kernel

//function definitions
void dedup(sc_packet &in, sc_packet &out);

#endif //DEDUP_H
