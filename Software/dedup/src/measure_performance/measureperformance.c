//
// Created by Ole Oggesen on 24.04.23.
//
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "evaluate.h"

/*
 * Used for performance evaluation of dedup program.
 */

int main(int argc, char **argv) {
    //setting defaults
    struct config config;
    config.log_file_path = "../../log";
    config.to_file = FALSE;
    config.repetitions = 50;
    config.program = "./dedup -c -i../inputs/partlyDuplicate.bin -o../outputs/partlyDuplicate.ddp -w none -v";

    //parsing config flags
    int option;
    while (-1 != (option = getopt(argc, argv, "f:p:r:"))) {
        switch (option) {
            case 'f':
                config.to_file = TRUE;
                config.log_file_path = optarg;
            break;
            case 'p':
                config.program = optarg;
            break;
            case 'r':
                config.repetitions = atoi(optarg);
            break;
            case '?':
                fprintf(stderr, "Unknown flag: %s", optarg);
                exit(EXIT_FAILURE);
            break;
        }
    }

    evaluate(config);

    exit(EXIT_SUCCESS);
}
