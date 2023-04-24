//
// Created by Ole Oggesen on 24.04.23.
//
#include "evaluate.h"

int evaluate(struct config config) {
    int r;
    //generate log file path
    char buff[30];
    time_t now = time(NULL);
    strftime(buff, 30, "%Y-%m-%d %H:%M:%S", localtime(&now));
    strcat(buff, ".log");
    char path_buff[40];
    strcpy(path_buff, config.log_file_path);
    strcat(path_buff, buff);

    //open log file
    FILE *log = stdout;
    if (config.to_file) {
        log = fopen(buff, "w+");
        if (log == NULL) {
            fprintf(stderr, "Could not open log file.");
            exit(EXIT_FAILURE);
        }
    }

    //measure elapsed time
    struct timeval tv_before, tv_after, tv_result;
    double total_time = 0;
    for (int i=0; i<config.repetitions; i++){
        FILE *program;

        r = gettimeofday(&tv_before, NULL);
        program = popen(config.program, "r");
        r += gettimeofday(&tv_after, NULL);

        fclose(program);

        timersub(&tv_after, &tv_before, &tv_result);
        total_time += (double) tv_result.tv_sec + (double) tv_result.tv_usec / 1000000;

        fprintf(log, "%ld.%06ld\n", (long int)tv_result.tv_sec, (long int)tv_result.tv_usec);
    }

    //print average
    double average_time = total_time / config.repetitions;
    fprintf(log, "\n%lf\n", average_time);

    //finishing
    if (config.to_file) fclose(log);

    exit(EXIT_SUCCESS);
}
