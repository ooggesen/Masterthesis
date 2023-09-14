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
#include "../top.hpp"

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


static void print_help(){
	cout << "Options:" << endl;
	cout << "-s <int>    Declare amount of data to read from input file." << endl;
	cout << "-d <string> Declare the type of data to read." << endl;
	cout << "              duplicate: Purely duplicate data." << endl;
	cout << "              semi: Random data with 50% duplication." << endl;
	cout << "              unique: Purely random data. Likely no duplicates." << endl;
}



int top_tb(int argc, char* argv[]){
	cout << "**********************************" << endl;
	cout << "     Testing whole top module     " << endl;
	cout << "**********************************" << endl;

	//declarations
	hls::stream< ap_uint< 64 > > test_data("test_data");
	hls::stream< ap_uint< 8 > > compare_data("compare_data");
	hls::stream< c_size_t > test_size("test_size"), compare_size("compare_size");
	hls::stream< bool > test_end("test_end");

	hls::stream< ap_uint< 64 > > out_stream("out_stream");
	hls::stream< bool > out_end("out_end");
	hls::stream< c_size_t > out_size("out_size");

	char data_path[200] = "/mnt/ole/Documents/coderepo/HLS/vitisHLS-implementation/data";
	char file_name[30] = "/duplicate.bin";
	int data_size = 1000;

	int errors = 0;
	bool end;

	//-------------------------------------------------
	cout << "Parsing input arguments." << endl << endl;
	int ch;
	while(-1 != (ch = getopt(argc, argv, "s:d:"))){
		switch(ch){
			case 's':
				data_size = stoi(optarg);
				cout << "Set test data size to " << stoi(optarg) << endl;
				break;
			case 'd':
				if (0 == strcmp(optarg, "duplicate")){
					strcpy(file_name, "/duplicate.bin" );
					cout << "Set data type to purely duplicate data." << endl;
				} else if (0 == strcmp(optarg, "semi")){
					strcpy(file_name, "/partlyDuplicate.bin");
					cout << "Set data type to semi duplicate data." << endl;
				} else if (0 == strcmp(optarg, "unique")){
					strcpy(file_name, "/random.bin");
					cout << "Set data type to purely unique data." << endl;
				} else {
					cout << "Wrong argument. The following options are allowed." << endl;
					cout << "duplicate: Purely duplicate data." << endl;
					cout << "semi: Random data with 50% duplication." << endl;
					cout << "unique: Purely random data. Likely no duplicates." << endl;
					return -1;
				}
				break;
			case 'h':
				print_help();
				break;
			default:
				cout << "Wrong input arguments." << endl;
				print_help();
				break;
		}
	}


	//-------------------------------------------------
	cout << "Reading data." << endl << endl;

	strcat(data_path, file_name);
	ifstream data_file(data_path, ios::in | ios::binary);
	if(data_file.is_open()){
		test_end.write(false);
		test_end.write(true);

		int b;
		int size = 0;
		ap_uint<64> buffer;
		while((b = data_file.get()) != EOF){
			if (size == MAX_FILE_SIZE || size == data_size){
				break;
			}

			compare_data.write(b);

			int pos = size % 8;
			buffer.range(7 + pos*8, pos*8) = b;
			if (pos == 7){
				test_data.write(buffer);
				buffer = 0;
			}

			size++;
		}
		//Write left over data
		if (buffer != 0){
			test_data.write(buffer);
		}

		test_size.write(size);
		compare_size.write(size);
	} else{
		cerr << "Could not open input file. Did you run ../test_data/main.c and moved the generated data to ./data ?" << endl;
		return -1;
	}

	compare_size.read();


	//-------------------------------------------------
	cout << "Running the dut." << endl << endl;

	top(test_data, test_size, test_end, out_stream, out_size, out_end);


	//-------------------------------------------------
	cout << "Checking results." << endl;
	if (strcmp(file_name, "/duplicate.bin") != 0){
		//Testbench for semi and random data
		end = out_end.read();

		while(!end){
			//Check header
			errors += check_header(out_stream);

			//Check data
			if (out_stream.empty()){
				cerr << "Top module produced no data." << endl << endl;
				errors++;
			}
			bool only_unique = true;
			int counter = 0;
			c_size_t size_buffer = 16;
			c_size_t size_boundary = out_size.read();
			while(size_boundary > size_buffer){
				cout << endl << "chunk nr. " << counter << "---------------------------------------" << endl;
				ap_uint< 64 > type;
				c_size_t size;
				read_seperator(out_stream, type, size);
				size_buffer += 16;

				//check type
				if (!(type == TYPE_COMPRESS || type == TYPE_FINGERPRINT)){
					cerr << "Wrong type information." << endl;
					errors++;
				}

				if (type == TYPE_COMPRESS){
					cout << "Found unique chunk." << endl;
				} else if (type == TYPE_FINGERPRINT){
					cout << "Found duplicate chunk." << endl;
				}

				if (only_unique && type == TYPE_FINGERPRINT){
					only_unique = false;
					cerr << "WARNING: Following data can not be checked." << endl; //since not sure how much must be read from compare_data
				}

				//check size
				if (size == 0){
					cerr << "Empty chunk output." << endl;
					errors++;
				}

				if (size != 20 && type == TYPE_FINGERPRINT){
					//hash must be 160 bit long
					cerr << "Wrong size for sha1 hash." << endl;
					errors++;
				}

				//Check data and empty out stream
				ap_uint< 192 > hash = 0;
				for (int i = 0 ; i < size.to_long() ; i += 8){
					size_buffer += 8;
					//build compare data in long format
					ap_uint< 64 > compare_long;
					ap_uint< 64 > out_long = out_stream.read();
					for (int j = 0 ; j < 8 ; j++){
						if (i + j < size.to_long())
							compare_long.range(7 + 8*j, 8*j) = compare_data.read();
						else
							compare_long.range(7 + 8*j, 8*j) = 0;
					}

					//check data with compare stream
					if (only_unique && type == TYPE_COMPRESS){
						if (out_long != compare_long){
							cerr << "Wrong data output." << endl;
							cerr << "Expected: " << hex << compare_long << endl;
							cerr << "Received: " << hex << out_long << endl;
							errors++;
						}
					}

					if (type == TYPE_FINGERPRINT){
						hash.range(63 + 8*i , 8*i) = out_long;
					}
				}

				if (type == TYPE_FINGERPRINT){
					cout << "Received hash: " << hash << endl;
				}
				counter++;
			}

			if (!only_unique){
				//compare data is not emptied correctly is duplicates are contained in testbench
				while(!compare_data.empty())
					compare_data.read();
			}

			end = out_end.read();
		}
	} else {
		//Purely duplicate testbench
		end = out_end.read();
		while(!end){
			errors += check_header(out_stream);

			ap_uint< 192 > last_hash = 0;
			c_size_t size_buffer = 16; // 16 bytes for header
			c_size_t size_boundary = out_size.read();
			int counter = 0;
			while(size_boundary > size_buffer){
					cout << endl << "chunk nr. " << counter << "---------------------------------------" << endl;
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
						cerr << "Wrong hash." << endl;
						cerr << "Might be edges of big chunk and no error." << endl;
					}

					last_hash = ap_uint< 192 >(hash); //if no duplicate sets last hash to 0 again
					counter++;
			}

			end = out_end.read();
		}

		//emptying compare_data which is not used in duplicate test bench
		while(!compare_data.empty()){
			compare_data.read();
		}
	}

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


