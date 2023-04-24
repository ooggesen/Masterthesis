//
// Created by Ole Oggesen on 24.04.23.
//

#ifndef DEDUP_CONFIG_H
#define DEDUP_CONFIG_H

#include <string.h>

#define TRUE 1
#define FALSE 0

struct config{
    int to_file;
    char *log_file_path;
    char *program;
    int repetitions;
};

#endif //DEDUP_CONFIG_H
