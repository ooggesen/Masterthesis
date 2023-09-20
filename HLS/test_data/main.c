#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_FILE_SIZE 10000000
//generate test data
int main() {
    //generating purely random data, with no duplicates
    FILE *fp = fopen("../random.bin", "wb");
    if(fp == NULL){
        fprintf(stderr, "Could not open file.\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL));
    for (int i = 0 ; i < MAX_FILE_SIZE ; i++){
        unsigned char random_char = rand() % 256;
        fwrite(&random_char, sizeof(random_char), 1, fp );
    }

    fclose(fp);

    //generating 50/50 duplicate/random data
    fp = fopen("../partlyDuplicate.bin", "wb");
    if(fp == NULL){
        fprintf(stderr, "Could not open file.\n");
        return EXIT_FAILURE;
    }

    srand(0);
    int boundary = 55;
    int r_counter = 0, d_counter = 0;
    for (int i = 0 ; i < MAX_FILE_SIZE ; i++){
        unsigned char surprise_char;
        if (r_counter >= boundary){
            if (rand() % 100 < 1){
                d_counter++;
        	}
                
            surprise_char = 255;
            
            if (d_counter >= boundary)
            	r_counter = 0;
        } else {
		    if (rand() % 100 < 1){
		    	d_counter = 0;
		    	r_counter++;
	    	}
		    	
        	surprise_char = rand() % 256;
		}
		
        fwrite(&surprise_char, sizeof(surprise_char), 1, fp );
    }

    fclose(fp);

    //generating purely duplicate data

    fp = fopen("../duplicate.bin", "wb");
    if(fp == NULL){
        fprintf(stderr, "Could not open file.\n");
        return EXIT_FAILURE;
    }

    for (int i = 0 ; i < MAX_FILE_SIZE ; i++){
        unsigned char duplicate_char = 255;
        fwrite(&duplicate_char, sizeof(duplicate_char), 1, fp );
    }

    fclose(fp);


    return 0;
}
