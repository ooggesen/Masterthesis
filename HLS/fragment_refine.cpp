/*
 * This file contains the fragment refine function which takes a big chunk and cuts it into smaller chunks
 */

#include "fragment_refine.h"

void read_in(bc_packet &in, bc_packet &buffer){
	copy(in, buffer);
}

void fragment_refine(bc_packet &in, sc_packet &out){
	//buffer
	bc_packet buffer;

	//read data in
	read_in(in, buffer);


}
