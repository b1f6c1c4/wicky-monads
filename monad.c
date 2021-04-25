#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "common.h"

pid_t child;

void trap(int sig) {
    kill(child, sig);
}

int main(int argc, char *argv[]) {
    struct cli_t cli;
    parse_cli(&cli, argc, argv,
              "Usage: monad [-t <limit>] -o [M:]<output> <input>... -- <executable> <arg>...\n"
              "    Run <executable> with <arg>... <input>...\n"
              "    Info from <input>.monad... will be read and failed ones will be skipped.\n"
              "    If the <executable> timeout/failed, <output>.monad will record the failure.\n"
              "    If the <executable> succeed, <output>.monad will record duration and mem.\n"
              "    If M:-prefix is specified, <output> itself will store such info instead.\n"
              "    In no case shall monad exit with a non-zero due to error from <output>.\n");
    return 0;
}
