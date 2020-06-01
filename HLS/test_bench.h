#ifndef TEST_BENCH_H
#define TEST_BENCH_H

#include "bus_def.h"
#include "hls_stream.h"
#include <stdlib.h>
#include <time.h>

#include <iostream>

void print_test_data(bus_packet test_data);
void generate_test_data(unsigned num_tests, bool set_duplicate, hls::stream< bus_packet > &test_data, hls::stream< bus_packet > &compare_data);
void shuffle(hls::stream< bus_packet > &sorted, hls::stream< bus_packet > &shuffeled);

#endif //TEST_BENCH_H
