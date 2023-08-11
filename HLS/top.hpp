/**
 * @file top.hpp
 *
 * @brief Top level function declaration
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */


#ifndef TOP_HPP
#define TOP_HPP

//Number of Threads
#define NP_REFINE 2
#define NP_MERGE NP_REFINE
//vitis hls includes
#define __gmp_const const
#include "ap_int.h"
#include "hls_stream.h"
#include "hls_math.h"

//user includes
#include "bus_def.hpp"
#include "fragment.hpp"
#include "fragment_refine.hpp"
#include "parsec/rabin.hpp"
#include "dedup.hpp"
#include "reorder.hpp"
#include "reorder_buffer.hpp"

/**
 * @brief Round robin merger.
 *
 * Not blocking of input does not contain data.
 * Must be defined in header for complience with undefined reference error.
 *
 * @param np Number of inputs
 */
template <typename M, typename D>
void merge(hls::stream< M > meta_in[],
		hls::stream< D > data_in[],
		hls::stream< bool > end_in[],
		int np,
		hls::stream< M > &meta_out,
		hls::stream< D > &data_out,
		hls::stream< bool > &end_out){
	bool end = false;
	bool end_np[NP_MERGE];

	init_end_buffer: for (int i = 0 ; i < np ; i++){
		end_np[i] = false;
	}

	merger: while(!end){
#pragma HLS LOOP_FLATTEN  off
		end = true;
		round_robin: for (int n = 0 ; n < np ; n++){
#pragma HLS LOOP_FLATTEN off
			if (!end_np[n]){
				end = false;

				if (!end_in[n].empty()){
					//check end condition
					if (end_in[n].read()){
						end_np[n] = true;
						continue;
					}

					end_out.write(false);

					M meta_data = meta_in[n].read();
					meta_out.write(meta_data);

					D buffer; //dummy buffer for length info
					merge_data: for (int i = 0 ; i < hls::ceil((double) meta_data.size.to_long()*8 / buffer.length()) ; i++){
#pragma HLS PIPELINE II=6
						data_out.write(data_in[n].read());
					}
				}
			}
		}
	}
	end_out.write(true);
}



/**
 * @brief Round robin splitter
 *
 *	type D must be of ap_(u)int type or return its length in bits with the call of the length() function.
 *	Must be implemented in header for complience with undefined reference error.
 *
 * @param np 	Number of outputs
 */
template <typename M, typename D>
void split(hls::stream< M > &meta_in,
		hls::stream< D > &data_in,
		hls::stream< bool > &end_in,
		int np,
		hls::stream< M > meta_out[],
		hls::stream< D > data_out[],
		hls::stream< bool > end_out[]){
	int n = 0;
	bool end = end_in.read();
	splitter: while(!end){
#pragma HLS LOOP_FLATTEN off
		end_out[n].write(false);

		M meta_data = meta_in.read();
		meta_out[n].write(meta_data);

		D buffer;
		for (int i = 0 ; i < hls::ceil((double)meta_data.size.to_long()*8 / buffer.length()) ; i++){
#pragma HLS PIPELINE II=6
			data_out[n].write(data_in.read());
		}
		n = ++n % np;

		end = end_in.read();
	}

	send_end_flag: for (int i = 0 ; i < np ; i++){
		end_out[i].write(true);
	}
}



/**
 * @brief Top level interace for the dedup kernel
 *
 * Since in dataflow region only one read and write access to an array is allowed we could not implement parallel dedup pipeline stages.
 * Refer to future work in thesis paper.
 *
 * @param in 		Input data stream
 * @param size_in	Size of input file
 * @param end_in	Indicates end of process after each file
 * @param out		Deduplicated output stream
 * @param size_out	Size of deduplicated file
 * @param end_out 	Indicates end of process after each file
 */
void top(hls::stream< ap_uint< 64 > > &in,
		hls::stream< c_size_t > &size_in,
		hls::stream< bool > &end_in,
		hls::stream< ap_uint< 64 > > &out,
		hls::stream< c_size_t > &size_out,
		hls::stream< bool > &end_out);


#endif //TOP_HPP
