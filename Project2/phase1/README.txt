# MyShell - Phase 1

# Overview
This program provides a basic Linux shell that runs built-in and external commands.

# Features
- Prompt: Displays CSE4100-SP-P2> for user input.
- Built-in Commands:
  - exit: Terminates the shell.
  - cd [{dir}]: Changes directory to {dir} or Home if no argument is given.
- External Commands: Supports ls, cat, echo, mkdir, rmdir, touch via Fork() and execvp().

# Implementation Details
- Structure: Follows the specified loop:
  1. Print prompt: printf("CSE4100-SP-P2> ").
  2. Read input: myshell_readinput() uses Fgets() to read from stdin.
  3. Parse input: myshell_parseinput() splits the input into an argument array.
  4. Execute: myshell_execute() handles built-ins or forks for external commands.
- Memory Management: Uses Malloc() and Free() to prevent leaks.
- Error Handling: Checks for EOF, command not found, and system call errors.

# Compilation
make

# Usage
./myShell
CSE4100-SP-P2> ls
CSE4100-SP-P2> mkdir myshell-dir
CSE4100-SP-P2> touch myshell-dir/cse4100
CSE4100-SP-P2> exit

# Bug Fixes
- Fixed child process hanging by adding exit(1) after execvp() failure.
- Added NULL check for args[0] to skip empty inputs.