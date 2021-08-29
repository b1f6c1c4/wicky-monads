#include <chrono>
#include <csignal>
#include <deque>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"
#include "config.h"

using json = nlohmann::json;
using namespace std::string_literals;

pid_t child{};
volatile std::sig_atomic_t timeout{};

void trap(int sig) {
    if (!child) return;
    if (sig == SIGALRM) {
        auto t = timeout;
        if (t == 0)
            kill(0, SIGINT);
        else if (t < 10)
            kill(0, SIGTERM);
        else
            kill(0, SIGKILL);
        timeout = t + 1;
        return;
    }
    kill(child, sig);
}

double sec(timeval t) {
    return static_cast<double>(t.tv_sec) + static_cast<double>(t.tv_usec) * 1e-6;
}
double sec(timespec t) {
    return static_cast<double>(t.tv_sec) + static_cast<double>(t.tv_nsec) * 1e-9;
}

void analysis(json &j) {
    if (j.contains("error"))
        j["status"] = "errored";
    else if (!j.contains("return"))
        j["status"] = "cancelled";
    else if (j.contains("timeout"))
        j["status"] = "timeout";
    else if (j.contains("signal"))
        j["status"] = "killed";
    else if (j["return"].get<int>())
        j["status"] = "failed";
    else
        j["status"] = "success";
}

int main(int argc, char *argv[]) {
    cli_t cli;
    parse_cli(&cli, argc, argv,
              "monad(1) from wicky-monads " PROJECT_VERSION "\n\n"
              "Usage: monad [-t <time-limit>] [-m <mem-limit>] [-M|--merge]\n"
              "             [-p|--partial] [-P|--panic] [-v]\n"
              "             -o <output> <input>... -- <executable> <arg>...\n"
              "    Run <executable> with <arg>... <input>... and redirect stdout to <output>\n"
              "    Info from <input>.monad... will be read and (with --partial) failed ones\n"
              "    will be skipped, or (no --partial) any faulty input will cancel the run.\n"
              "    If the <executable> timeout/failed, <output>.monad will record the case.\n"
              "    If the <executable> succeed, <output>.monad will record duration and mem.\n"
              "    If --merge is specified, <output> itself will store such info instead.\n"
              "    If --panic is given, successful run will return 0, failed/cancelled will\n"
              "    return 1, errored will return 2. If --panic is not given, it will always\n"
              "    return 0 unless errored. <output> will be created/removed according to the\n"
              "    return value of monad (0 = always exist, 1/2 = always removed).\n");

    auto fst = cli.output + (cli.merge_output ? ""s : ".monad"s);
    try {
        std::ofstream ofst{ fst };
        if (!ofst.good())
            throw std::runtime_error{ "Not good" };
    } catch (const std::exception &e) {
        std::cerr << "Cannot open output file " << fst << ": " << e.what() << "\n";
        return 1;
    }

    bool maybe_good = true;
    json j{ { "good", false } };

    int fd;
    int status;
    bool to;
    decltype(std::chrono::high_resolution_clock::now()) start;

    try {
        std::vector<const char *> args;
        for (size_t i{}; i < cli.n_args; i++)
            args.emplace_back(cli.args[i]);

        j["parsing"]["time-limit-sec"] = sec(cli.time_limit);
        j["parsing"]["mem-limit-MiB"] = static_cast<double>(cli.mem_limit) / 1024.0 / 1024.0;
        j["parsing"]["merge-output"] = cli.merge_output;
        j["parsing"]["output"] = cli.output;
        j["parsing"]["monad-output"] = fst;
        j["parsing"]["executable"] = cli.executable;

        j["error"]["stage"] = "defs";
        for (size_t i{}; i < cli.n_defs; i++) {
            std::string_view sv{ cli.defs[i] };
            auto id = sv.find('=');
            if (id == std::string_view::npos) {
                j[cli.defs[i]] = nullptr;
                continue;
            }
            j[std::string{ sv.substr(0, id) }] = cli.defs[i] + id + 1;
        }

        j["error"]["stage"] = "input";
        auto any_good = false;
        auto any_bad = false;
        for (size_t i{}; i < cli.n_inputs; i++) {
            auto &jj = j["inputs"][i];
            jj["name"] = cli.inputs[i];
            std::ifstream ifst{ std::string{ cli.inputs[i] } + ".monad" };
            bool good;
            if (ifst.good()) {
                json st;
                ifst >> st;
                ifst.close();
                good = st["good"].get<bool>();
                jj["status"] = std::move(st);
            } else {
                std::ifstream ifs{ cli.inputs[i] };
                good = ifs.good();
            }
            if (good) {
                any_good = true;
                args.emplace_back(cli.inputs[i]);
            } else {
                any_bad = true;
            }
        }
        args.emplace_back(nullptr);
        if (!(cli.n_inputs == 0
            || cli.partial && any_good
            || !cli.partial && !any_bad)) {
            maybe_good = false;
            if (!cli.panic) {
                j["error"]["stage"] = "cancelled-open";
                fd = open(cli.output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd == -1)
                    throw std::runtime_error{ "Cannot open: "s + std::strerror(errno) };
                j["error"]["stage"] = "close";
                if (close(fd) == -1)
                    throw std::runtime_error{ "Cannot close: "s + std::strerror(errno) };
            }
            goto cancelled;
        }

        j["error"]["stage"] = "dump-args";
        for (auto a : args)
            if (a)
                j["parsing"]["args"].emplace_back(a);
            else
                j["parsing"]["args"].emplace_back(nullptr);

        j["error"]["stage"] = "open";
        fd = open(cli.output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1)
            throw std::runtime_error{ "Cannot open: "s + std::strerror(errno) };
        j["error"]["stage"] = "dup2";
        if (dup2(fd, STDOUT_FILENO) == -1)
            throw std::runtime_error{ "Cannot dup2: "s + std::strerror(errno) };
        j["error"]["stage"] = "close";
        if (close(fd) == -1)
            throw std::runtime_error{ "Cannot close: "s + std::strerror(errno) };

        j["error"]["stage"] = "timing-start";
        start = std::chrono::high_resolution_clock::now();

        j["error"]["stage"] = "sig";
        signal(SIGHUP, &trap);
        signal(SIGINT, &trap);
        signal(SIGQUIT, &trap);
        signal(SIGABRT, &trap);
        signal(SIGKILL, &trap);
        signal(SIGTERM, &trap);
        signal(SIGUSR1, &trap);
        signal(SIGUSR2, &trap);
        signal(SIGALRM, &trap); // for receiving timer

        j["error"]["stage"] = "fork";
        if ((child = fork()) == -1)
            throw std::runtime_error{ "Cannot fork: "s + std::strerror(errno) };
        if (!child) { // I'm the child!
            if (cli.mem_limit) {
                rlimit64 rlim{ cli.mem_limit * 95 / 100, cli.mem_limit };
                setrlimit64(RLIMIT_DATA, &rlim);
            }
            nice(19);
            execvp(cli.executable, const_cast<char * const *>(args.data()));
            fprintf(stderr, "execvp: %d: %s\n", errno, std::strerror(errno));
            exit(127);
        }

        j["error"]["stage"] = "timer";
        if (cli.time_limit.tv_sec || cli.time_limit.tv_nsec) {
            timer_t tmr;
            sigevent_t sev{ SIGEV_SIGNAL, SIGALRM };
            if (timer_create(CLOCK_REALTIME, &sev, &tmr))
                throw std::runtime_error{ "Cannot timer_create: "s + std::strerror(errno) };
            itimerspec spec{ { 1, 0 }, cli.time_limit };
            if (timer_settime(tmr, 0, &spec, nullptr))
                throw std::runtime_error{ "Cannot timer_settime: "s + std::strerror(errno) };
        }

        j["error"]["stage"] = "waitpid";
        if (waitpid(child, &status, WUNTRACED) == -1)
            throw std::runtime_error{ "Cannot waitpid: "s + std::strerror(errno) };
        to = timeout;

        {
            j["error"]["stage"] = "timing-stop";
            auto end = std::chrono::high_resolution_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            j["resource"]["wall-time-sec"] = static_cast<double>(dur.count()) / 1e9;
        }

        j["error"]["stage"] = "check-status";
        if (WIFSIGNALED(status)) {
            auto sig= WTERMSIG(status);
            j["signal"]["value"] = sig;
            j["signal"]["string"] = strsignal(sig);
            if ((sig == SIGKILL || sig == SIGTERM || sig == SIGINT) && to)
                j["timeout"] = true;
            j["return"] = 128 | sig;
            maybe_good = false;
        } else {
            if (!WIFEXITED(status))
                throw std::runtime_error{ "Cannot waitpid: "s + std::strerror(errno) };
            j["return"] = WEXITSTATUS(status);
            if (WEXITSTATUS(status))
                maybe_good = false;
        }

        {
            j["error"]["stage"] = "check-resource";
            rusage ru;
            if (getrusage(RUSAGE_CHILDREN, &ru))
                throw std::runtime_error{ "Cannot getrusage: "s + std::strerror(errno) };
            j["resource"]["user-mode-time-sec"] = sec(ru.ru_utime);
            j["resource"]["kernel-mode-time-sec"] = sec(ru.ru_stime);
            j["resource"]["max-rss-MiB"] = static_cast<double>(ru.ru_maxrss) / 1024.0;
        }

        if (cli.merge_output) {
            j["error"]["stage"] = "read-output";
            std::ifstream ifs{ cli.output };
            if (!ifs.good()) {
                j["output"] = nullptr;
            } else {
                try {
                    json jj;
                    ifs >> jj;
                    j["output"] = jj;
                } catch (const nlohmann::detail::parse_error &) {
                    ifs.seekg(std::ios::beg);
                    using I = std::istreambuf_iterator<char>;
                    j["output"] = std::string{ I{ ifs }, I{} };
                }
            }
        }

        cancelled:
        j["error"]["stage"] = "write-output";
        std::ofstream ofs{ fst };
        if (!ofs.good())
            throw std::runtime_error{ "Not good" };
        j["good"] = maybe_good;
        j.erase("error");
        analysis(j);
        if (!cli.verbose) j.erase("parsing");
        ofs << j << "\n";
        if (cli.panic && !maybe_good) {
            unlink(cli.output);
            return 1;
        }
        return 0;
    } catch (const std::exception &e) {
        j["error"]["what"] = e.what();
        j["error"]["errno"]["value"] = errno;
        j["error"]["errno"]["string"] = std::strerror(errno);
        goto error_write_j;
    }

    error_write_j:
    unlink(cli.output);
    std::cerr << "Error during " << j["error"]["stage"] << ": " << j["error"] << "\n";
    std::ofstream ofst{ fst };
    analysis(j);
    if (ofst.good()) {
        ofst << j << "\n";
        std::cerr << "Notice: Log has been written to " << fst << "\n";
    } else {
        std::cerr << "Fatal: Still not good, showing the log:\n" << std::setw(2) << j;
    }
    return 2;
}
