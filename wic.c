#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"

pid_t child;

void trap(int sig) {
    if (!child) return;
    kill(child, sig);
}

int main(int argc, char *argv[]) {
    struct cli_t cli;
    parse_cli(&cli, argc, argv,
              "Usage: wic -o <output> -- <executable> <args>...\n"
              "    Run the <executable> with <args>..., redirect its output to <output> if\n"
              "    and only if their content differs.\n");
    if (cli.merge_output) exit(125);
    if (cli.time_limit.tv_sec || cli.time_limit.tv_nsec) exit(125);
    if (cli.mem_limit) exit(125);
    if (access(cli.output, R_OK | W_OK) == -1) {
        int fd = open(cli.output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) return 123;
        if (dup2(fd, STDOUT_FILENO) == -1) return 122;
        close(fd);
        execvp(cli.executable, (char * const *)cli.args);
        return 127;
    }
    int fds[2];
    if (pipe(fds) == -1)
        return 126;

    signal(SIGHUP, &trap);
    signal(SIGINT, &trap);
    signal(SIGQUIT, &trap);
    signal(SIGABRT, &trap);
    signal(SIGKILL, &trap);
    signal(SIGTERM, &trap);
    signal(SIGUSR1, &trap);
    signal(SIGUSR2, &trap);
    if ((child = fork()) == -1) return 128;
    if (!child) {
        while ((dup2(fds[1], STDOUT_FILENO) == -1) && (errno == EINTR));
        close(fds[1]);
        close(fds[0]);
        execvp(cli.executable, (char * const *)cli.args);
        return 127;
    }
    close(fds[1]);

    int fd = open(cli.output, O_RDWR);
    if (fd == -1) {
        unlink(cli.output);
        return 123;
    }

    char bufa[4096], bufb[4096];
    bool flag = false;
    while (true) {
        ssize_t cnta = read(fds[0], bufa, sizeof(bufa));
        if (cnta == -1 && errno == EINTR) continue;
        if (cnta == -1) return 125;
        if (!cnta) break;
        if (!flag)
            for (size_t o = 0; o < cnta;) {
                ssize_t cntb = read(fd, bufb, cnta - o);
                if (cntb == -1 && errno == EINTR) continue;
                if (cntb == -1) return 125;
                if (!cntb) { flag = true; break; }
                if (memcmp(bufa + o, bufb, cntb) != 0) {
                    flag = true;
                    lseek(fd, -cntb, SEEK_CUR);
                    break;
                }
                o += cntb;
            }
        if (flag) write(fd, bufa, cnta);
    }
    off_t pos = lseek(fd, 0, SEEK_CUR);
    if (pos != lseek(fd, 0, SEEK_END))
        ftruncate(fd, pos);
    close(fd);

    int status;
    if (waitpid(child, &status, WUNTRACED) == -1) {
        unlink(cli.output);
        return 126;
    }

    if (WIFSIGNALED(status)) {
        unlink(cli.output);
        return 128 | WTERMSIG(status);
    }

    if (!WIFEXITED(status)) {
        unlink(cli.output);
        return 124;
    }
    return WEXITSTATUS(status);
}
