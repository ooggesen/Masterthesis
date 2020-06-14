#ifndef TEST_BENCH_H
#define TEST_BENCH_H

//user libraries
#include "bus_def.hpp"
//vitis libraries
#include "hls_stream.h"
//std libraries
#include <stdlib.h>
#include <time.h>

#include <iostream>

//helper functions
void print_test_data(sc_packet test_data);
void print_test_data(bc_packet test_data);
void generate_test_data(unsigned num_tests, hls::stream< bc_packet > &test_data, hls::stream< bc_packet > &compare_data);
void generate_test_data(unsigned num_tests, bool set_duplicate, hls::stream< sc_packet > &test_data, hls::stream< sc_packet > &compare_data);
void shuffle(hls::stream< sc_packet > &sorted, hls::stream< sc_packet > &shuffeled);

//test benches
int dedup_tb();
int reorder_tb();
int bram_tb();
int copy_tb();
int reorder_buffer_tb();
int rabin_tb();
int fragment_refine_tb();

#endif //TEST_BENCH_H
