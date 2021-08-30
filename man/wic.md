% WIC(1) wic v0.1 | wicky-monads v0.1 manual
% b1f6c1c4 <b1f6c1c4@gmail.com>
% August 2021

# NAME

**wic** - Redirect stdout but overwrite only if content differs

# SYNOPSIS

**wic** `-o` `<output>` `--` `<executable>` `[<args>...]`

# DESCRIPTION

Shell-based stdout redirection will call **openat(3)** with **O_WRONLY|O_CREAT|O_TRUNC**, causing **mtime** to be changed.
This may not be a problem until redirection is used in mtime-based make systems, including but not limited to **make(1)**.
**wic** solves the problem of redirection by comparing the content of the existing file against the data from stdout, and if any difference is noticed, write the data into the file (which cause **mtime** change).
If there is no difference, **mtime** stays the same.

**wic** is helpful when your build system has *restat* functionality,
i.e. it double checks whether the output file is actually rewritten by the command or not.
An incomplete list of such build systems includes **make(1)** and **ninja(1)**.

# OPTION

**-h**, `--help`
: Show brief usage information.

**-o**
: The file to be written into.
It should a regular file, but other files may also work.
If the file does not exist, **wic** acts as the same as shell-bsed stdout redirection.

# NOTES

If the executable has a non-zero exit status,
the output file will be unconditionally removed.
If **wic** itself errored,
the output file will be removed too.

# EXIT STATUS

The same as running the executable, if not any error:

120
: Unable to **waitpid(2)** on the child process.

121
: Unable to **read(2)** from the pipe.

122
: Unable to duplicate an open file descriptor (**dup2(3)**).

123
: Unable to **open(2)** the output file.

124
: Unable to determind child process exit reason.

125
: Invalid options specified.

126
: Unable to create a **pipe(2)**.

127
: Unable to call **exec(3)**.

128
: Unable to **fork(2)**.

>128
: Child process terminated due to a signal.
Specially, if SIGKILL received, 128+9=137 will be the case.

# SEE ALSO

**cmp(1)**
