# MyShell - Phase 3

# Overview
This program extends the Linux shell to support background jobs, signal handling, and job management, along with pipelines and quoted inputs.

# Features
- Prompt: Displays CSE4100-SP-P2> for user input.
- Built-in Commands:
  - exit: Terminates the shell.
  - cd [{dir}]: Changes directory to {dir} or Home if no argument is given.
  - jobs: Lists running and stopped background jobs.
  - bg %[{jod_id}]: Resumes a stopped job in the background.
  - fg %[{job_id}]: Brings a job to the foreground.
  - kill %[{job_id}]: Terminates a job.

- External Commands: Supports ls, cat, echo, mkdir, rmdir, touch via Fork() and execvp().
- Pipelines: Handles commands like ls | grep file using pipe() for inter-process communication.
- Quated Imputs: Supports quoted inputs like "ls -al | grep file" to treat spaces and special characters as part of arguments.
- Background Jobs: Runs commands in the background with & (e.g., sleep 10 &).
- Signal Handling: Manages SIGINT (^C), SIGTSTP (^Z), and SIGCHLD for job control.

# Implementation Details
- Structure: Follows the specified loop:
  1. Print prompt: printf("CSE4100-SP-P2> ").
  2. Read input: myshell_readinput() uses Fgets() with tcsetpgrp() for terminal control.
  3. Parse input: myshell_parseinput() splits input into a 3D argument array, handling pipes, quotes, and &.
  4. Execute: myshell_execute() manages pipelines, forks, and job states (FG/BG).
- Job Management: Uses a volatile Job struct linked list to track jobs, with add_job(), delete_job(), and update_job_state().
- Signal Handling: Implements sigchld_handler(), sigint_handler(), and sigtstp_handler() for job state changes.
- Memory Management: Uses Malloc() and Free() to prevent leaks, with free_args() for cleanup.
- Error Handling: Checks for EOF, command not found, pipe errors, and signal errors.

# Compilation
make

# Usage
./myShell
CSE4100-SP-P2> ls
CSE4100-SP-P2> ls -al {filename} | grep {filename} &
CSE4100-SP-P2> cat {filename} | grep -i "abc" &
CSE4100-SP-P2> jobs
CSE4100-SP-P2> sleep 100
CSE4100-SP-P2> ^Z
CSE4100-SP-P2> exit