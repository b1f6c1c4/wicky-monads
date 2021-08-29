#include "common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct timespec parse_time(const char *str) {
    long total = 0;
    long res = 0;
    for (; *str; str++) {
        if (isdigit(*str))
            res *= 10, res += *str - '0';
        else if (*str == ':')
            total = (total + res) * 60, res = 0;
        else if (*str == '.')
            break;
        else
            exit(125);
    }
    struct timespec ts;
    ts.tv_sec = total + res;
    ts.tv_nsec = 0;
    if (!*str) return ts;
    str++;
    for (long n = 100000000; *str; str++, n /= 10) {
        if (isdigit(*str))
            ts.tv_nsec += n * (*str - '0');
        else
            exit(125);
    }
    return ts;
}

size_t parse_mem(const char *str) {
    char *end;
    double total = strtod(str, &end);
    if (end[0] == '\0') goto done;
    double base = end[1] == 'i' ? 1024.0 : 1000.0;
    switch (end[0]) {
        default: exit(125);
        case 'Y': total *= base;
        case 'Z': total *= base;
        case 'E': total *= base;
        case 'P': total *= base;
        case 'T': total *= base;
        case 'G': total *= base;
        case 'M': total *= base;
        case 'k':
        case 'K': total *= base;
    }
    if (end[1] == 'i') end++;
    end++;
    if (!(end[0] == '\0' || end[0] == 'B' && end[1] == '\0'))
        exit(125);
    done:
    return (size_t)total;
}

void parse_cli(struct cli_t *res, int argc, char *argv[], const char *help) {
    res->time_limit.tv_sec = res->time_limit.tv_nsec = 0;
    res->mem_limit = 0;
    res->merge_output = false;
    res->output = NULL;
    res->n_inputs = 0;
    res->inputs = calloc(argc, sizeof(const char *));
    res->n_defs = 0;
    res->defs = calloc(argc, sizeof(const char *));
    res->verbose = false;
    res->partial = false;
    res->panic = false;
    for (argc--, argv++; argc; argc--, argv++) {
        if (argv[0][0] != '-') {
            res->inputs[res->n_inputs++] = argv[0];
            continue;
        }
        if (argv[0][1] == 'D') {
            res->defs[res->n_defs++] = argv[0] + 2;
            continue;
        }
        if (strcmp(argv[0], "--help") == 0) goto help;
        if (strcmp(argv[0], "--version") == 0) goto help;
        if (strcmp(argv[0], "--merge") == 0) {
            res->merge_output = true;
            continue;
        }
        if (strcmp(argv[0], "--partial") == 0) {
            res->partial = true;
            continue;
        }
        if (strcmp(argv[0], "--panic") == 0) {
            res->panic = true;
            continue;
        }
        if (argv[0][1] == '\0' || argv[0][2] != '\0')
            exit(125);
        switch (argv[0][1]) {
            case 'M':
                res->merge_output = true;
                continue;
            case 'v':
                res->verbose = true;
                continue;
            case 'p':
                res->partial = true;
                continue;
            case 'P':
                res->panic = true;
                continue;
            case 'h':
            help:
                fprintf(stderr, "%s", help);
                exit(0);
            case 't':
                res->time_limit = parse_time(argv[1]);
                argc--, argv++;
                continue;
            case 'm':
                res->mem_limit = parse_mem(argv[1]);
                argc--, argv++;
                continue;
            case 'o':
                res->output = argv[1];
                argc--, argv++;
                continue;
            case '-':
                argc--, argv++;
                goto done;
            default:
                exit(125);
        }
    }
    done:
    res->executable = argv[0];
    if (!res->executable || !res->output)
        exit(125);
    res->n_args = argc;
    res->args = (const char **)argv;
}
