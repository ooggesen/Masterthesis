#include <stdio.h>
#include <stdlib.h>

#define MAX_FILE_SIZE 1000000
//generate test data
int main() {
    //generating purely random data, with no duplicates
    FILE *fp = fopen("../random.bin", "wb");
    if(fp == NULL){
        fprintf(stderr, "Could not open file.\n");
        return EXIT_FAILURE;
    }

    srand(0);
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
    int counter = 0;
    for (int i = 0 ; i < MAX_FILE_SIZE ; i++){
        unsigned char surprise_char;
        if (counter >= 2){
            if (rand() % 101 < 25)
                counter = 0;
                
            surprise_char = 255;
        } else {
        	surprise_char = rand() % 256;
		}
        fwrite(&surprise_char, sizeof(surprise_char), 1, fp );
        
        if (rand() % 101 < 50)
        	counter++;
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
