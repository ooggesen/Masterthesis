/*
 * @brief Contains top functions for testing of round robin schedulers
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#include "scheduler.hpp"



void top_split(
		hls::stream< sc_packet > &in,
		hls::stream< c_data_t > &data_in,
		hls::stream< bool > &end_in,
		hls::stream< sc_packet > out[NP_MERGE],
		hls::stream< c_data_t > data_out[NP_MERGE],
		hls::stream< bool > end_out[NP_MERGE]){
	split< sc_packet , c_data_t >(in, data_in, end_in, out, data_out, end_out);
}



void top_merge(
		hls::stream< sc_packet > in[NP_MERGE],
		hls::stream< c_data_t > data_in[NP_MERGE],
		hls::stream< bool > end_in[NP_MERGE],
		hls::stream< sc_packet > &out,
		hls::stream< c_data_t > &data_out,
		hls::stream< bool > &end_out){
	merge< sc_packet ,  c_data_t >(in, data_in, end_in, out, data_out, end_out);
}
