# smallsh

A shell implemented in C for Operating Systems 1 at Oregon State University.

The general syntax for the shell is: $ command [arg1 arg2 ...] [< input_file] [> output_file] [&]

Features include: I/O redirection, foreground or background processes, custom signal handling, toggable foreground only mode (Ctrl C), built in commands (status, exit and cd). All other commands are executed by a child process using the execvp function.

I received an 100% on the assignment.

compile with gcc -std=c99
