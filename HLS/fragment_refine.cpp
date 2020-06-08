/*
 * This file contains the fragment refine function which takes a big chunk and cuts it into smaller chunks
 */

#include "fragment_refine.hpp"

void read_in(bc_packet &in, bc_packet &buffer){
	buffer = bc_packet(in);
}

void fragment_refine(bc_packet &in, sc_packet &out){
	//buffer
	bc_packet buffer;

	//read data in
	read_in(in, buffer);


}
