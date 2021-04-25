#pragma once

#include <stdbool.h>
#include <time.h>

struct cli_t {
    unsigned long limit;
    bool merge_output;
    const char *output;
    size_t n_inputs;
    const char **inputs;
    const char *executable;
    size_t n_args;
    const char **args;
};

void parse_cli(struct cli_t *res, int argc, char *argv[], const char *help);
