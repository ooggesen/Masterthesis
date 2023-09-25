/*
 * @brief Contains the definitions of round robin schedulers
 *
 * @author Ole Oggesen
 * @bugs No known bugs
 */

#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "bus_def.hpp"
#include "top.hpp"

#include "hls_stream.h"

#define NP_MERGE NP_REFINE //maximum with of split and merge arrays

/**
 * @brief Round robin merger.
 *
 * Not blocking of input does not contain data.
 * Must be defined in header for complience with undefined reference error.
 */
template <typename M, typename D>
void merge(hls::stream< M > meta_in[NP_MERGE],
		hls::stream< D > data_in[NP_MERGE],
		hls::stream< bool > end_in[NP_MERGE],
		hls::stream< M > &meta_out,
		hls::stream< D > &data_out,
		hls::stream< bool > &end_out){
	static bool end = false;
	static bool end_np[NP_MERGE];

	if (!end){
		end = true;
		round_robin: for (int n = 0 ; n < NP_MERGE ; n++){
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
						data_out.write(data_in[n].read());
					}
				}
			}
		}
	}

	if (end)
		end_out.write(true);
}



/**
 * @brief Round robin splitter
 *
 *	type D must be of ap_(u)int type or return its length in bits with the call of the length() function.
 *	Must be implemented in header for complience with undefined reference error.
 */
template <typename M, typename D>
void split(hls::stream< M > &meta_in,
		hls::stream< D > &data_in,
		hls::stream< bool > &end_in,
		hls::stream< M > meta_out[NP_MERGE],
		hls::stream< D > data_out[NP_MERGE],
		hls::stream< bool > end_out[NP_MERGE]){
	static bool end = false, send_end = true;

	if (!end && !end_in.empty()){
		for (int n = 0 ; n < NP_MERGE ; n++){
			end = end_in.read();
			if (end){
				send_end_flag: for (int i = 0 ; i < NP_MERGE ; i++){
					end_out[i].write(true);
				}
				break;
			}

			end_out[n].write(false);

			M meta_data = meta_in.read();
			meta_out[n].write(meta_data);

			D buffer;
			for (c_size_t i = 0 ; i < meta_data.size ; i += buffer.length() / 8){
				data_out[n].write(data_in.read());
			}
		}
	}
}



//TOP FUNCTIONS


/*
 * @brief Top function for testing of the split function.
 *
 * Consists only of the round robin scheduler called splitter, which is part of the top module.
 *
 * @param in meta data input
 * @param data_in data input
 * @param end_in signaling true after end of process
 * @param out meta data output, array width NP_MERGE
 * @param data_out data output, array width NP_MERGE
 * @param end_out signaling true after end of process, array width NP_MERGE
 */
void top_split(
		hls::stream< sc_packet > &in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool > &end_in,
		hls::stream< sc_packet > out[NP_MERGE],
		hls::stream< c_data_t > data_out[NP_MERGE],
		hls::stream< bool > end_out[NP_MERGE]);



/*
 * @brief Top function for testing of the merge function.
 *
 * Consists only of the round robin scheduler called merger, which is part of the top module.
 *
 * @param in meta data input, array width NP_MERGE
 * @param data_in data input, array width NP_MERGE
 * @param end_in signaling true after end of process, array width NP_MERGE
 * @param out meta data output
 * @param data_out data output
 * @param end_out signaling true after end of process
 */
void top_merge(
		hls::stream< sc_packet > in[NP_MERGE],
		hls::stream< c_data_t > data_in[NP_MERGE],
		hls::stream< bool > end_in[NP_MERGE],
		hls::stream< sc_packet > &out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out);


#endif //SCHEDULER_HPP
