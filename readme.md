#CSC 222 - Simple Shell Project

## Name
Aayusha Khadka

## Description
This project is a simple shell written in C called "techshell".
It works similar to a basic Linux terminal. The shell keeps running until
the user type "exit".

The shell:
- Displays the current working directory as the prompt
- Accepts user commands
- Executes normal Linux command using fork() and execvp()
- Supports input redirection using <
- Supports output redirection using >
- Implements the built-in cd command
- Prints proper error messages using errno

## compiling
gcc techshell.c -o techshell

## to run use
./techshell

## working features
- prompt updates correctly
- cd command works
- Normal commands like ls, pwd, cat, wc work
- Output redirection (>) works
- Input redirection (<) works
- invalid commands show error messages

## features that does not work
 -Pipes(|)
- Background processes(&)
