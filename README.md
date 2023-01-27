# Small Shell

This is a very tiny shell written in C which has the following functionality:

1. Provides a prompt for running commands
2. Handles blank lines and comments, which are lines beginning with the # character
3. Provides expansion for the variable $$ (PID)
4. Executes 3 commands exit, cd, and status via code built into the shell
5. Executes other commands by creating new processes using a function from the exec family of functions
6. Supports input and output redirection
7. Supports running commands in foreground and background processes

# To run:

1. To compile the code, put the makefile and smallsh.c into the same directory
2. CD into this directory and type "make"
3. You can then run "./smallsh", and enter commands
