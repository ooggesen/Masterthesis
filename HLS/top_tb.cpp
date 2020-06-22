/*
 * This file contains all testbenches for the top module. This refers to testbenches that check intermediate results as well as the whole programm.
 * -> top functions which output intermediate results, must be created by adapting the top function to output intermediate values
 */


#include "test_bench.hpp"
#include "top.hpp"

#define NUM_TESTS 2
using namespace std;

static int check_header(hls::stream< ap_uint< 8 > > &out_stream){
	int errors = 0;
	ap_uint< 32 > checkbit;
	for (int i = 0 ; i < 4 ; i++){ //TODO assume LSB first to send
		checkbit.range(7 + i*8, i*8) = out_stream.read();
	}
	if (checkbit != CHECKBIT){
		cout << "Wrong checkbit." << endl;
		errors++;
	}

	ap_uint< 8 > compress_type = out_stream.read();
	if (compress_type != COMPRESS_NONE){
		cout << "Wrong compress type." << endl;
		errors++;
	}

	return errors;
}



static void read_seperator(hls::stream< ap_uint< 8 > > &out_stream, ap_uint< 8 > &type, c_size_t &size){
	type = out_stream.read();

	for (int i = 0 ; i < W_CHUNK_SIZE / 8 ; i++){
		size.range(7 + i*8, i*8) = out_stream.read();
	}
}


/*
 * This testbench tests a top module without the dedup kernel
 * -> adapt the top function accordingly
 * -> remove print header and print seperator functions from the reorder kernel
 */
int top_refine_tb(){
	cout << "*********************************************************" << endl;
	cout << "   Testing whole module until fragment refine kernel     " << endl;
	cout << "*********************************************************" << endl;

	//Generating input data
	cout << "Generating enough data for at least " << NUM_TESTS << " big chunk segments for the top function." << endl << endl;

	hls::stream< ap_uint< 8 > > test_data, compare_data;
	generate_test_data(NUM_TESTS, test_data, compare_data);

	//Running
	cout << "Running the dut." << endl << endl;

	hls::stream< ap_uint< 8 > > out_stream;
	top(test_data, true, out_stream);

	//Checking results
	int counter = 0, errors = 0;
	while(!out_stream.empty()){
		//cout << "Checking the " << counter << "th small chunk.-----" << endl;

		ap_uint< 8 > compare_byte = compare_data.read();
		ap_uint< 8 > out_byte = out_stream.read();

		if (out_byte != compare_byte){
			cout << "Wrong data output." << endl;
			cout << "Byte pos: " << counter << " Expected: " << hex << compare_byte << ",but received " << out_byte << endl;
			errors++;
		}
		counter++;
	}

	if (!compare_data.empty()){
		cout << "Lost data along pipeline." << endl;
		errors++;
	}

	if (errors == 0){
		cout << "All tests passed." << endl;
	} else {
		cout << "Test failed." << endl;
	}


	return errors;
}



/*
 * This testbench tests the whole top module with all components
 * -> can only test data coherency up the the first data duplicate
 */
int top_tb(){
	cout << "**********************************" << endl;
	cout << "     Testing whole top module     " << endl;
	cout << "**********************************" << endl;


	//FIRST PHASE -> no duplicate input
	cout << "Testing on random output. Likely no duplicates." << endl << endl;

	//Generating input data
	cout << "Generating enough data for at least " << NUM_TESTS << " big chunk segments for the top function." << endl << endl;

	hls::stream< ap_uint< 8 > > test_data, compare_data;
	generate_test_data(NUM_TESTS, test_data, compare_data);

	//Running
	cout << "Running the dut." << endl << endl;

	hls::stream< ap_uint< 8 > > out_stream;
	top(test_data, true, out_stream);


	//Checking
	cout << "Checking results." << endl;
	int errors = 0;

	//Check header
	errors += check_header(out_stream);

	//Check data
	bool only_unique = true;
	int counter = 0;
	while(!out_stream.empty()){
		ap_uint< 8 > type;
		c_size_t size;
		read_seperator(out_stream, type, size);

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
		ap_uint< 160 > hash = 0;
		for (int i = 0 ; i < size.to_long() ; i++){
			ap_uint< 8 > compare_byte = compare_data.read();
			ap_uint< 8 > out_byte = out_stream.read();
			if (only_unique && type == TYPE_COMPRESS){
				if (out_byte != compare_byte){
					cout << "Wrong data output." << endl;
					errors++;
				}
			}

			if (type == TYPE_FINGERPRINT){
				hash.range(7 + 8*i , 8*i);
			}
		}

		if (type == TYPE_FINGERPRINT){
			cout << "Received hash: " << hash << endl;
		}
		counter++;
	}

	if (only_unique && !compare_data.empty()){
		cout << "Data lost. Compare stream is not empty. Leftover data: " << endl;
		while(!compare_data.empty()){
			cout << hex << compare_data.read() << endl;
		}
		errors++;
	}

	//SECOND PHASE -> only duplicates
	cout << endl << endl << "Generating purely duplicate data." << endl << endl;

	cout << "Generating enough data for at least " << NUM_TESTS << " big chunk segments for the top function." << endl << endl;

	unsigned char input = 255;
	for (int i = 0 ; i < 1.1*NUM_TESTS*BIG_CHUNK_SIZE/8 ; i++){
		test_data.write(input);
	}

	//Running
	cout << "Running the dut." << endl << endl;
	top(test_data, true, out_stream);

	//Checking
	cout << "Checking results." << endl;
	errors += check_header(out_stream);

	ap_uint< 160 > last_hash = 0;
	while(!out_stream.empty()){
			ap_uint< 8 > type;
			c_size_t size;
			read_seperator(out_stream, type, size);

			//After first run, check that only duplicates come in
			if (last_hash != 0 && (type != TYPE_FINGERPRINT || size != 20)){
				cout << "Detected non duplicate data in purely duplicate data stream." << endl;
				errors++;
			}

			//Check data
			ap_uint< 160 > hash = 0;
			for (int i = 0 ; i < size.to_long() ; i ++){
				ap_uint< 8 > out_byte = out_stream.read();

				if (type == TYPE_FINGERPRINT){
					hash.range(7 + 8*i , 8*i);
				} else {
					if (out_byte != input){
						cout << "Wrong data." << endl;
						errors++;
					}
				}
			}

			if (last_hash != 0 && last_hash != hash){
				cout << "Wrong hash." << endl;
				errors++;
			}

			last_hash = ap_uint< 160 >(hash);
	}

	return errors;
}
