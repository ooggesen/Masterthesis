/*
 * @file top_tb.cpp
 *
 * @brief Contains testbench for the top level function.
 *
 * Tests two cases. Input is complete unique and input is completely indentical.
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */


#include "test_bench.hpp"
#include "top.hpp"

#define NUM_TESTS 2
using namespace std;

static int check_header(hls::stream< ap_uint< 64 > > &out_stream){
	int errors = 0;
	ap_uint< 64 > checkbit = out_stream.read();
	if (checkbit != CHECKBIT){
		cout << "Wrong checkbit in header." << endl;
		errors++;
	}

	ap_uint< 64 > compress_type = out_stream.read();
	if (compress_type != COMPRESS_NONE){
		cout << "Wrong compress type." << endl;
		errors++;
	}

	return errors;
}



static void read_seperator(hls::stream< ap_uint< 64 > > &out_stream, ap_uint< 64 > &type, c_size_t &size){
	type = out_stream.read();
	size = out_stream.read();
}



int top_tb(){
	cout << "**********************************" << endl;
	cout << "     Testing whole top module     " << endl;
	cout << "**********************************" << endl;


	//FIRST PHASE -> no duplicate input
	cout << "Testing on random output. Likely no duplicates." << endl << endl;

	//Generating input data
	cout << "Generating enough data for at least " << NUM_TESTS << " big chunk segments for the top function." << endl << endl;

	hls::stream< ap_uint< 64 > > test_data("test_data");
	hls::stream< ap_uint< 8 > > compare_data("compare_data");
	hls::stream< c_size_t > test_size("test_size"), compare_size("compare_size");
	hls::stream< bool > test_end("test_end");
	generate_test_data(NUM_TESTS, test_data, compare_data, test_size, compare_size, test_end);
	compare_size.read();

	//Running
	cout << "Running the dut." << endl << endl;

	hls::stream< ap_uint< 64 > > out_stream("out_stream");
	hls::stream< bool > out_end("out_end");
	hls::stream< c_size_t > out_size("out_size");
	top(test_data, test_size, test_end, out_stream, out_size, out_end);


	//Checking
	cout << "Checking results." << endl;
	int errors = 0;

	bool end = out_end.read();

	while(!end){
	//Check header
	errors += check_header(out_stream);

	//Check data
	if (out_stream.empty()){
		cout << "Top module produced no data." << endl << endl;
		errors++;
	}
	bool only_unique = true;
	int counter = 0;
	c_size_t size_buffer = 16;
	c_size_t size_boundary = out_size.read();
	while(size_boundary > size_buffer){
			ap_uint< 64 > type;
			c_size_t size;
			read_seperator(out_stream, type, size);
			size_buffer += 16;

			//check type

			if (!(type == TYPE_COMPRESS || type == TYPE_FINGERPRINT)){
				cout << "Wrong type information." << endl;
				errors++;
			}

			if (only_unique && type == TYPE_FINGERPRINT){
				only_unique = false;
				cout << "Duplicate data chunk found." << endl;
				cout << "Following data can not be checked." << endl;
			}

			//check size
			if (size == 0){
				cout << "Empty chunk output." << endl;
				errors++;
			}

			if (size != 20 && type == TYPE_FINGERPRINT){
				//hash must be 160 bit long
				cout << "Wrong size for sha1 hash." << endl;
				errors++;
			}

			//Check data and empty out stream
			ap_uint< 192 > hash = 0;
			for (int i = 0 ; i < size.to_long() ; i += 8){
				size_buffer += 8;
				ap_uint< 64 > compare_long;
				ap_uint< 64 > out_long = out_stream.read();
				for (int j = 0 ; j < 8 ; j++){
					if (i + j < size.to_long())
						compare_long.range(7 + 8*j, 8*j) = compare_data.read();
					else
						compare_long.range(7 + 8*j, 8*j) = 0;
				}
				if (only_unique && type == TYPE_COMPRESS){
					if (out_long != compare_long){
						cout << "Wrong data output." << endl;
						cout << "Expected: " << hex << compare_long << endl;
						cout << "Received: " << hex << out_long << endl;
						errors++;
					}
				}

				if (type == TYPE_FINGERPRINT){
					hash.range(63 + 8*i , 8*i);
				}
			}

			if (type == TYPE_FINGERPRINT){
				cout << "Received hash: " << hash << endl;
			}
			counter++;
		}

		end = out_end.read();
	}
	/*
	//SECOND PHASE -> only duplicates
	cout << endl << endl << "Generating purely duplicate data." << endl << endl;

	cout << "Generating enough data for at least " << NUM_TESTS << " big chunk segments for the top function." << endl << endl;

	ap_uint< 64 > input;
	for (int i = 0 ; i < 8 ; i++){
		input.range(7 + 8*i, 8*i) = 255;
	}

	cout << "Every input is " << hex << input << endl;

	c_size_t file_length = 1.1*NUM_TESTS*BIG_CHUNK_SIZE/8;
	file_length -= file_length % 8;
	for (int i = 0 ; i < file_length.to_long() ; i += 8){
		test_data.write(input);
	}

	//Running
	cout << "Running the dut." << endl << endl;
	test_size.write(file_length);
	test_end.write(false);
	test_end.write(true);

	top(test_data, test_size, test_end, out_stream, out_size, out_end);

	//Checking
	cout << "Checking results." << endl;
	end = out_end.read();
	while(!end){
		errors += check_header(out_stream);

		ap_uint< 192 > last_hash = 0;
		c_size_t size_buffer = 16; // 16 bytes for header
		c_size_t size_boundary = out_size.read();
		while(size_boundary > size_buffer){
				ap_uint< 64 > type;
				c_size_t size;
				read_seperator(out_stream, type, size);
				size_buffer += 16; // 16 bytes for seperator

				if (type == TYPE_FINGERPRINT){
					cout << "Found duplicate chunk." << endl;
				} else {
					cout << "Found unique chunk." << endl;
				}

				//Check data
				ap_uint< 192 > hash = 0;
				c_data_t buffer = 0;
				for (int i = 0 ; i < size.to_long() ; i += 8){
					size_buffer += 8;
					ap_uint< 64 > out_long = out_stream.read();

					if (type == TYPE_FINGERPRINT){
						hash.range(63 + i*8 , i*8) = out_long;
					} else {
						int pos = (i/8) % (1024/64);

						if (i != 0 && pos == 0)
							cout << hex << buffer << endl;

						buffer.range(63 + 64*pos, 64*pos) = out_long;
					}
				}

				if (type == TYPE_FINGERPRINT && last_hash != 0 && (hash == 0 || last_hash != hash)){
					cout << "Wrong hash." << endl;
					cout << "Might be end of big chunk and no error." << endl;
				}

				last_hash = ap_uint< 192 >(hash); //if no duplicate sets last hash to 0 again
		}

		end = out_end.read();
	}
	*/

	return errors;
}


int only_fragment_tb(){
	cout << "*************************************************************" << endl;
	cout << "     Testing top module without dedup and reorder stage.     " << endl;
	cout << "*************************************************************" << endl;

	cout << "Generating enough data for at least " << NUM_TESTS << " big chunk segments for the top function." << endl << endl;


	hls::stream< ap_uint< 64 > > test_data("test_data");
	hls::stream< ap_uint< 8 > > compare_data("compare_data");
	hls::stream< c_size_t > test_size("test_size"), compare_size("compare_size");
	hls::stream< bool > test_end("test_end");
	generate_test_data(NUM_TESTS, test_data, compare_data, test_size, compare_size, test_end);
	compare_size.read();

	//Running
	cout << "Running the dut." << endl << endl;

	hls::stream< ap_uint< 64 > > out_stream("out_stream");
	hls::stream< bool > out_end("out_end");
	hls::stream< c_size_t > out_size("out_size");
	top(test_data, test_size, test_end, out_stream, out_size, out_end);


	//Checking
	cout << "Checking results." << endl;
	int errors = 0;

	while(!out_end.read()){
		c_size_t size = out_size.read();
		for (int i = 0 ; i < hls::ceil((double) size / 8) ; i++){
			out_stream.read();
		}
	}

	while(!compare_data.empty()){
		compare_data.read();
	}

	int counter = 0;
	while(!out_stream.empty()){
		out_stream.read();
		counter++;
	}
	if (counter != 0){
		cout << counter << " elements left in out stream fifo." << endl;
		errors++;
	}


	return errors;
}
