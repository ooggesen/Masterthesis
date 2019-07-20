#ifndef TEST_BENCH_HPP
#define TEST_BENCH_HPP

//user libraries
#include "bus_def.hpp"
#include "reorder_buffer.hpp"
//vitis libraries
#define __gmp_const const
#include "ap_int.h"
#include "hls_stream.h"
#include "hls_math.h"
//std libraries
#include <stdlib.h>
#include <time.h>

#include <iostream>

//helper functions
void print_test_data(sc_packet test_data);
void print_test_data(bc_packet test_data);
void generate_test_data(unsigned num_tests,
		hls::stream< ap_uint< 64 > > &test_data,
		hls::stream< ap_uint< 8 > > &compare_data,
		hls::stream< c_size_t > &test_size,
		hls::stream< c_size_t > &compare_size,
		hls::stream< bool > &end);
void generate_test_data(unsigned num_tests,
		hls::stream< bc_packet > &test_meta,
		hls::stream< c_data_t > &test_data,
		hls::stream< bool > &test_end,
		hls::stream< bc_packet > &compare_meta,
		hls::stream< ap_uint< 8 > > &compare_data);
void generate_test_data(unsigned num_tests,
		bool set_duplicate,
		hls::stream< sc_packet > &test_meta,
		hls::stream< c_data_t > &test_data,
		hls::stream< bool > &test_end,
		hls::stream< sc_packet > &compare_meta,
		hls::stream< c_data_t > &compare_data);
void shuffle(hls::stream< sc_packet > &sorted_meta,
		hls::stream< c_data_t > &sorted_data,
		hls::stream< bool > &sorted_end,
		hls::stream< sc_packet > &shuffeled_meta,
		hls::stream< c_data_t > &shuffeled_data,
		hls::stream< bool > &shuffeled_end);

//test benches
int dedup_tb();
int reorder_tb();
int bram_tb();
int copy_tb();
int reorder_buffer_tb();
int fragment_refine_tb();
int fragment_tb();
int top_tb();
int generate_test_data_tb();
int shuffle_tb();
int is_equal_tb();

#endif //TEST_BENCH_HPP
