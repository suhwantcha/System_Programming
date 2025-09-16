# MyShell - Phase 2

# Overview
This program extends the Linux shell to support command pipelines, quoted inputs, and improved parsing for built-in and external commands.

# Features
- Prompt: Displays CSE4100-SP-P2> for user input.
- Built-in Commands:
  - exit: Terminates the shell.
  - cd [{dir}]: Changes directory to {dir} or Home if no argument is given.
- External Commands: Supports ls, cat, echo, mkdir, rmdir, touch via Fork() and execvp().
- Pipelines: Handles commands like ls | grep file using pipe() for inter-process communication.
- Quated Imputs: Supports quoted inputs like "ls -al | grep file" to treat spaces and special characters as part of arguments.

# Implementation Details
- Structure: Follows the specified loop:
  1. Print prompt: printf("CSE4100-SP-P2> ").
  2. Read input: myshell_readinput() uses Fgets() to read from stdin.
  3. Parse input: myshell_parseinput() splits input into a 3D argument array, handling pipes and quotes.
  4. Execute: myshell_execute() manages pipelines with pipe(), Dup2(), and forks for each command.
- Memory Management: Uses Malloc() and Free() to prevent leaks, with free_args() for cleanup.
- Error Handling: Checks for EOF, command not found, pipe errors, and system call errors.

# Compilation
make

# Usage
./myShell
CSE4100-SP-P2> ls
CSE4100-SP-P2> ls | grep {filename}
CSE4100-SP-P2> cat {filename} | less
CSE4100-SP-P2> cat {filename} | grep -i "abc" | sort -r
CSE4100-SP-P2> exit

# Bug Fixes
 - Fixed parsing to handle quoted arguments by detecting single/double quotes.
 - Resolved pipeline issues by properly closing pipe file descriptors after Dup2().