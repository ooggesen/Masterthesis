/**
 * @file test_bench.hpp
 *
 * @brief Contains function declarations for the testbench helper functions as well as the testbenches itself.
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#ifndef TEST_BENCH_HPP
#define TEST_BENCH_HPP

//user libraries
#include "../bus_def.hpp"
#include "../top.hpp"
//vitis libraries
#define __gmp_const const
#include "ap_int.h"
#include "hls_stream.h"
#include "hls_math.h"
//std libraries
#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <fstream>
#include <string.h>
#include <unistd.h>

/*
 * @brief Prints out the values of a small chunk packet type
 *
 * @param test_data bus packet to print
 */
void print_test_data(sc_packet test_data);



/*
 * @brief Prints out the values of a big chunk packet type
 *
 * @param test_data: bus packet to print
 */
void print_test_data(bc_packet test_data);



/**
 * @brief Generates test data for top and fragment pipeline stage
 */
void generate_test_data(unsigned num_tests,
		hls::stream< ap_uint< 64 > > &test_data,
		hls::stream< ap_uint< 8 > > &compare_data,
		hls::stream< c_size_t > &test_size,
		hls::stream< c_size_t > &compare_size,
		hls::stream< bool > &end);


/**
 * @brief generates test data for the fragment_refine pipeline stage
 */
void generate_test_data(unsigned num_tests,
		hls::stream< bc_packet > &test_meta,
		hls::stream< c_data_t > &test_data,
		hls::stream< bool > &test_end,
		hls::stream< bc_packet > &compare_meta,
		hls::stream< ap_uint< 8 > > &compare_data);


/**
 * @brief Gnerates test data for the dedup and reorder pipeline stages
 */
void generate_test_data(unsigned num_tests,
		bool set_duplicate,
		hls::stream< sc_packet > &test_meta,
		hls::stream< c_data_t > &test_data,
		hls::stream< bool > &test_end,
		hls::stream< sc_packet > &compare_meta,
		hls::stream< c_data_t > &compare_data);



/**
 * @brief shuffles the input streams for usage in the reorder pipeline stage
 */
void shuffle(hls::stream< sc_packet > &sorted_meta,
		hls::stream< c_data_t > &sorted_data,
		hls::stream< bool > &sorted_end,
		hls::stream< sc_packet > &shuffeled_meta,
		hls::stream< c_data_t > &shuffeled_data,
		hls::stream< bool > &shuffeled_end);

//TESTBENCHES

/*
 * @brief This testbench tests the whole top module with all components.
 *
 * This testbench is seperated into two parts. First the top module is tested with purely random data, which with the current random seed does not
 * contain any duplicate chunks.
 * The second test, tests with purely duplicate data. There should only be 3 different unique chunks. The small chunks which are contained in a big chunk,
 * the small chunk at the end of a complete big chunk.
 * And the small chunk which concludes the test, since the data is not divisible by the big chunk size.
 */
int top_tb(int argc, char* argv[]);

int only_fragment_tb();
int dedup_tb();
int reorder_tb();
int bram_tb();
int copy_tb();
int reorder_buffer_tb();
int fragment_refine_tb();
int fragment_tb();
int generate_test_data_tb();
int shuffle_tb();
int is_equal_tb();
int split_tb();
int merge_tb();

#endif //TEST_BENCH_HPP
