#pragma once

#include <time.h>

#ifdef __cplusplus
#include <cstdbool>
#include <cstddef>
#else
#include <stdbool.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct cli_t {
    struct timespec time_limit;
    size_t mem_limit;
    bool merge_output;
    const char *output;
    size_t n_inputs;
    const char **inputs;
    const char *executable;
    size_t n_args; // includes args[0]
    const char **args; // args[0] == executable
    bool verbose;
    bool partial;
    bool panic;
    size_t n_defs;
    const char **defs;
};

void parse_cli(struct cli_t *res, int argc, char *argv[], const char *help);

#ifdef __cplusplus
}
#endif
