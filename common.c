#include "common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t parse_limit(const char *str) {
    size_t total = 0;
    size_t res = 0;
    for (; *str; str++) {
        if (isdigit(*str))
            res *= 10, res += *str;
        else if (*str == ':')
            total = (total + res) * 60, res = 0;
        else
            exit(125);
    }
    return total + res;
}

void parse_cli(struct cli_t *res, int argc, char *argv[], const char *help) {
    if (argc < 5)
        exit(125);
    res->n_inputs = 0;
    res->inputs = calloc(argc, sizeof(const char *));
    for (argc--; argc; argc--, argv++) {
        if (argv[0][0] != '-') {
            res->inputs[res->n_inputs++] = argv[0];
            continue;
        }
        if (strcmp(argv[0], "--help") == 0) goto help;
        if (argv[0][1] == '\0' || argv[0][2] != '\0')
            exit(125);
        switch (argv[0][1]) {
            case 'h':
            help:
                fprintf(stderr, "%s", help);
                exit(0);
            case 't':
                res->limit = parse_limit(argv[1]);
                argv++;
                continue;
            case 'o':
                res->output = argv[1];
                argv++;
                continue;
            case '-':
                goto done;
            default:
                exit(125);
        }
    }
    done:
    res->executable = argv[0];
    if (!res->executable || !res->output)
        exit(125);
    res->n_args = --argc;
    res->args = (const char **)++argv;
}
