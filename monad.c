#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

pid_t child;

void trap(int sig) {
    kill(child, sig);
}

/* Usage: monad [-t <limit>] -o [M:]<output> <input>... -- <executable> <arg>...
 *     Run <executable> with <arg>... <input>...
 *     Info from <input>.monad... will be read if exists.
 *     If the <executable> timeout/failed, <output>.monad will record the failure.
 *     If the <executable> succeed, <output>.monad will record duration and mem.
 *     If M:-prefix is specified, <output> itself will store such info instead.
 *     In no case shall monad exit with a non-zero due to error from <output>.
 */
int main(int argc, char *argv[]) {
    if (argc < 4)
        return 127;
    if (strcmp(argv[1], "-o") != 0)
        return 125;
    if (access(argv[2], R_OK | W_OK) == -1) {
        int fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) return 123;
        if (dup2(fd, STDOUT_FILENO) == -1) return 122;
        close(fd);
        execvp(argv[3], argv + 3);
        return 127;
    }
    int fds[2];
    if (pipe(fds) == -1)
        return 126;
    if ((child = fork()) == -1) return 128;
    if (!child) {
        while ((dup2(fds[1], STDOUT_FILENO) == -1) && (errno == EINTR));
        close(fds[1]);
        close(fds[0]);
        execvp(argv[3], argv + 3);
        return 127;
    }
    close(fds[1]);

    signal(SIGHUP, &trap);
    signal(SIGINT, &trap);
    signal(SIGQUIT, &trap);
    signal(SIGABRT, &trap);
    signal(SIGKILL, &trap);
    signal(SIGTERM, &trap);
    signal(SIGUSR1, &trap);
    signal(SIGUSR2, &trap);

    int fd = open(argv[2], O_RDWR);
    if (fd == -1) {
        unlink(argv[2]);
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
        unlink(argv[2]);
        return 126;
    }

    if (WIFSIGNALED(status)) {
        unlink(argv[2]);
        return 128 | WTERMSIG(status);
    }

    if (!WIFEXITED(status)) {
        unlink(argv[2]);
        return 124;
    }
    return WEXITSTATUS(status);
}
