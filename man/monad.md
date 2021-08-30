% MONAD(1) monad v0.1 | wicky-monads v0.1 manual
% b1f6c1c4 <b1f6c1c4@gmail.com>
% August 2021

# NAME

**monad** - Error-tolerant build system helper

# SYNOPSIS

**monad** `[options]` `-o` `<output>` `<input>...` `--` `<executable>` `[<arg>...]`

# MOTIVATION

Sometimes we want to track, manage, or even introduce failures in build systems.
If a single step fails, its following steps may:

1. Abort running (e.g. it further processes the failed step's output)
1. Proceed, but with less inputs (e.g. it kicks out the failed one in tournament comparison)
1. Proceed as usual (e.g. it logs previous failure into database)

Most build systems, including **make(1)** and **ninja(1)**, supports only the first case.

Besides, it is not easy to log meta data (including execution time, memory consumption, etc.)
nor setup time/memory limitation on the execution of commands.

**monad** provides a build-system-agnostic way to
track, manage, and even introduce failures
at target-level granularity.

# DESCRIPTION

**monad** can be treated as an enhanced way to run
*`<executable> <arg>... <input>... > <output>`*.
Differences are summarized as below.

1. A JSON file *`<output>.monad`* will be created
that logs all the meta data of the invocation.
If a corresponding file from an input (*`<input>.monad`*) exists, it will be analyzed.

1. If all the inputs were successful, **monad** will run the *`<executable>`*.
If not, the behavior depends on whether **`--partial`** is specified:
without **`--partial`**, **monad** will cancel the run;
with **`--partial`**, **monad** will continue to run the *`<executable>`*,
but supplying it with only the successful inputs.

1. The exit status of **monad** itself is configurable thru **`--panic`** option.
See EXIT STATUS section for more information.

# OPTION

**-h**, **`--help`**
: Show brief usage information.

**-v**
: Display more intermediate information.

**-o**
: The file to redirect *`<executable>`*'s stdout into.
It should a regular file, but other files may also work.

**-t** `<time-limit>`
: The maximum wall-clock time *`<executable>`* is allowed to run,
after which SIGINT, SIGTERM, SIGKILL will be sent to terminate the process.

**-m** `<mem-limit>`
: The maximum memory *`<executable>`* is allowed to use.
See **setrlimit(3)** for more information.

**-M**, `--merge`
: Whether to combine *`<output>.monad`* with *`<output>`*.
Without **`--merge`**, there will be two files and
*`<output>.monad`* will contain meta data in JSON
while *`<output>`* will contain raw program output.
With **`--merge`**, there will be only one file
*`<output>`* that contains a JSON with both the meta data
and program output as a string.

**-p**, `--partial`
: Whether unsuccessful inputs will cancel the run.
Without **`--partial`**, **monad** will cancel the run.
With **`--partial`**, **monad** will continue to run the *`<executable>`*,
but supplying it with only the successful inputs.

**-P**, `--panic`
: Skipping it will make it **monad** always return 0
when *`<executable>`* isn't successful.
See EXIT STATUS section for more information.

# NOTES

If **`--merge`** is *not* specified,
`<output>` will be created/removed according to the
return value of **monad**: 0 = always exist, otherwise = always removed.
`<output>.monad` will always exist unless **monad** encounters internal error.

If **`--merge`** is specified,
`<output>` will always exist unless **monad** encounters internal error.

# EXIT STATUS

See the below table for exit status.
*Failed* means that *`<executable>`* quitted with non-zero return value (not due to a signal).
*Killed* means that *`<executable>`* quitted due to a signal,
including the case of exceeding memory limit.
*Cancelled* means at least one input wasn't successful and **`--partial`** was not specified.

| Status | with **-P** | without **-P** |
|---|---|---|
| Succeed | 0 | 0 |
| Failed | 1 | 0 |
| Timeout | 1 | 0 |
| Killed | 1 | 0 |
| Cancelled | 1 | 0 |
| Errored | 2 | 2 |

# SEE ALSO

**timeout(1)**, **time(1)**
