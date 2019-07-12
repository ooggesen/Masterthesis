/*
 * This file contains all the helper functions for the testbenches
 */


#include "test_bench.hpp"
using namespace std;



/*
 * prints out the values of a small chunk packet type
 *
 * @param test_data: bus packet to print
 */
void print_test_data(sc_packet test_data){
	cout << "-----" << endl;

	cout << dec <<"size: " << test_data.size <<  endl;
	cout << "hash in hex: " << hex << test_data.hash << endl;
	cout << dec << "l1 pos: " << test_data.l1_pos << endl;
	cout << "l2 pos: " << test_data.l2_pos << endl;
	cout << "dup: " << test_data.is_duplicate << endl;

	cout << "-----" << endl;
}



/*
 * prints out the values of a big chunk packet type
 *
 * @param test_data: bus packet to print
 */
void print_test_data(bc_packet test_data){
	cout << "-----" << endl;

	cout << dec <<"size: " << test_data.size <<  endl;
	cout << dec << "l1 pos: " << test_data.l1_pos << endl;

	cout << "-----" << endl;
}



void generate_test_data(unsigned num_tests,
		hls::stream< ap_uint< 64 > > &test_data,
		hls::stream< ap_uint< 8 > > &compare_data,
		hls::stream< c_size_t > &test_size,
		hls::stream< c_size_t > &compare_size,
		hls::stream< bool > &end){
	srand(time(NULL));

	int file_length = (int) 1.2*BIG_CHUNK_SIZE/8*num_tests;
	for (int i = 0 ; i < hls::ceil((double) file_length/8) ; i++){
		ap_uint< 64 > buffer;
		for (int j = 0 ; j < 8 ; j++){
			if (i*8 + j < file_length){
				ap_uint< 8 > byte = rand() % 256;
				buffer.range(7 + 8*j, 8*j) = byte;
				compare_data.write(byte);
			} else {
				buffer.range(7 + 8*j, 8*j) = 0;
			}
		}


		test_data.write(buffer);
	}
	test_size.write(file_length);
	compare_size.write(file_length);
	end.write(false);

	end.write(true);
}



void generate_test_data(unsigned num_tests,
		hls::stream< bc_packet > &test_meta,
		hls::stream< c_data_t > &test_data,
		hls::stream< bc_packet > &compare_meta,
		hls::stream< c_data_t > &compare_data){
	srand(time(NULL));

	unsigned l1 = 0;
	for (int td = 0 ; td < num_tests ; td++){
		bc_packet packet;
		packet.size = (BIG_CHUNK_SIZE - 10*SMALL_CHUNK_SIZE + rand() % (40 * SMALL_CHUNK_SIZE))/8;
		packet.l1_pos = l1++;

		//print_test_data(packet);

		//pushing generated meta on stream
		test_meta.write(packet);
		compare_meta.write(packet);

		//generating data and writing data
		for (int elem = 0 ; elem < hls::ceil((double) packet.size.to_long()*8 / W_DATA) ; elem++){
			c_data_t data_buffer;
			for (int byte_pos = 0 ; byte_pos < W_DATA/8 ; byte_pos++){
				if (packet.size.to_long() > elem*W_DATA/8 + byte_pos){
					ap_uint< 8 > random_byte = rand() % (1 << 7);
					data_buffer.range(7 + 8*byte_pos , 8*byte_pos) = random_byte;
				} else {
					data_buffer.range(7 + 8*byte_pos , 8*byte_pos) = 0;
				}
			}

			compare_data.write(data_buffer);
			test_data.write(data_buffer);
		}
	}
}



void generate_test_data(unsigned num_tests,
		bool set_duplicate,
		hls::stream< sc_packet > &test_meta,
		hls::stream< c_data_t > &test_data,
		hls::stream< sc_packet > &compare_meta,
		hls::stream< c_data_t > &compare_data){
	c_data_t data_buffer[SC_STREAM_SIZE];
	srand(time(NULL));

	unsigned l1 = 0, l2 = 0;
	sc_packet last;
	for (int td = 0; td < num_tests; td ++){
		sc_packet packet;

		//increment hash as a filler
		packet.hash = td;

		//in 20 percent of cases create duplicate for test -> output the hash value
		bool is_duplicate = false;
		if (rand() % 101 < 20 && td != 0){
			//copy the last value
			packet = sc_packet(last);
			packet.hash = td; //convenient for checking in the results

			is_duplicate = true;

			for (int elem  = 0 ; elem < hls::ceil((double) packet.size.to_long()*8 / W_DATA) ; elem++){
				c_data_t current_data = data_buffer[elem];

				test_data.write(current_data);
				compare_data.write(current_data);
			}
		} else {
			packet.size = (rand() % 24) + SMALL_CHUNK_SIZE/8 - 12; //small variances in size

			is_duplicate = false;

			//generating data
			for (int elem = 0 ; elem < hls::ceil((double) packet.size.to_long()*8 / W_DATA)  ; elem++){
				for (int seg = 0 ; seg < W_DATA/8 ; seg++){
					//write to buffer
					if (packet.size.to_long() > elem*W_DATA/8 + seg){
						ap_uint< 32 > random_data = rand() % (1 << 7);
						data_buffer[elem].range(7 + 8*seg, 8*seg) = random_data;
					} else
						data_buffer[elem].range(7 + 8*seg, 8*seg) = 0;
				}
				//write out
				test_data.write(data_buffer[elem]);
				compare_data.write(data_buffer[elem]);
			}
		}

		//update small chunk positions
		packet.l2_pos = l2;
		packet.l1_pos = l1;
		bool is_last_l2_chunk = false;
		// increment the l1 positions after an average big chunk if transmitted
		// modified by chance
		if (l2 * SMALL_CHUNK_SIZE >= BIG_CHUNK_SIZE && rand() % 101 < 30) {
			l2 = 0;
			l1++;
			is_last_l2_chunk = true;
		} else {
			l2++;
		}

		packet.last_l2_chunk = is_last_l2_chunk;

		//set duplicate flag if wanted
		if (set_duplicate){
			packet.is_duplicate = is_duplicate;
		} else {
			packet.is_duplicate = false;
		}

		//write data to corresponding stream
		test_meta.write(packet);

		packet.is_duplicate = is_duplicate;
		compare_meta.write(packet);

		//print out correctly set bus packet
		//print_test_data(packet);

		last = sc_packet(packet);
	}
}



void shuffle(hls::stream< sc_packet > &sorted_meta,
		hls::stream< c_data_t > &sorted_data,
		hls::stream< sc_packet > &shuffeled_meta,
		hls::stream< c_data_t > &shuffeled_data){
	hls::stream< sc_packet > meta_buffer("meta_buffer");
	hls::stream< c_data_t > data_buffer("data_buffer");
	int in_buffer = 0;
	while(!sorted_meta.empty() || !meta_buffer.empty()){
		unsigned percent = rand() % 101;
		if (!sorted_meta.empty() && percent < 33){
			//write directly from input to output
			sc_packet packet = sorted_meta.read();
			shuffeled_meta.write(packet);
			for (int i = 0  ; i < hls::ceil((double) packet.size.to_long()*8 / W_DATA) ; i++){
				shuffeled_data.write(sorted_data.read());
			}
		} else if (in_buffer >= BUFFER_SIZE_1 ||
				in_buffer >= BUFFER_SIZE_2 ||
				(!meta_buffer.empty() && (percent < 66 || sorted_meta.empty()))){
			//write from buffer to output
			sc_packet packet = meta_buffer.read();
			shuffeled_meta.write(packet);

			for (int i = 0 ; i < hls::ceil((double) packet.size.to_long()*8 / W_DATA) ; i ++){
				shuffeled_data.write(data_buffer.read());
			}
			in_buffer--;
		} else if (!sorted_meta.empty()){
			//write from input to buffer
			sc_packet packet = sorted_meta.read();
			meta_buffer.write(packet);

			for (int i = 0 ; i < hls::ceil((double) packet.size.to_long()*8 / W_DATA) ; i ++){
				data_buffer.write(sorted_data.read());
			}
			in_buffer++;
		}
	}
	return;
}
