#define _CRT_SECURE_NO_WARNINGS
#include "myshell.h"
#include <errno.h>

int main() {
    char *input;
    int cmd_count;

    do {
        printf("CSE4100-SP-P2> ");
        input = myshell_readinput();
        if (!input)
            continue;

        char ***args = myshell_parseinput(input, &cmd_count);
        if (cmd_count == 0) {
            free_args(args, cmd_count);
            Free(input);
            continue;
        }

        myshell_execute(args, cmd_count);
        free_args(args, cmd_count);
        Free(input);
    } while (1);
    return 0;
}

char *myshell_readinput(void) {
    char *input = Malloc(MAXLINE);
    if (Fgets(input, MAXLINE, stdin) == NULL) {
        Free(input);
        return NULL;
    }
    return input;
}

char ***myshell_parseinput(char *input, int *cmd_count) {
    char ***args = Malloc(MAXCMDS * sizeof(char **));
    for (int i = 0; i < MAXCMDS; i++) {
        args[i] = Malloc(MAXARGS * sizeof(char *));
        for (int j = 0; j < MAXARGS; j++) {
            args[i][j] = NULL;
        }
    }

    char *buf = input;
    int cmd_idx = 0, argc = 0;
    buf[strlen(buf) - 1] = ' ';
    while (*buf && (*buf == ' '))
        buf++;

    char *delim;
    while (*buf && cmd_idx < MAXCMDS - 1) {
        if (*buf == '"' || *buf == '\'') {
            char quote = *buf++;
            delim = strchr(buf, quote);
            if (!delim)
                delim = buf + strlen(buf);
        } else {
            delim = strpbrk(buf, " |");
            if (!delim)
                delim = buf + strlen(buf);
        }
        if (delim == buf) {
            if (*buf == '|') {
                if (argc > 0) {
                    args[cmd_idx][argc] = NULL;
                    cmd_idx++;
                    argc = 0;
                }
                buf++;
            }
            while (*buf && (*buf == ' '))
                buf++;
            continue;
        }
        args[cmd_idx][argc] = Malloc(delim - buf + 1);
        strncpy(args[cmd_idx][argc], buf, delim - buf);
        args[cmd_idx][argc][delim - buf] = '\0';
        argc++;
        buf = delim;
        if (*buf == '"' || *buf == '\'') {
            buf++;
        } else if (*buf == '|') {
            args[cmd_idx][argc] = NULL;
            cmd_idx++;
            argc = 0;
            buf++;
        }
        while (*buf && (*buf == ' '))
            buf++;
    }
    if (argc > 0) {
        args[cmd_idx][argc] = NULL;
        cmd_idx++;
    }
    *cmd_count = cmd_idx;
    if (cmd_idx == 0)
        *cmd_count = 0;
    return args;
}

void myshell_execute(char ***args, int cmd_count) {
    int p[2], fd = STDIN_FILENO, status;
    pid_t pid;

    if (cmd_count == 0)
        return;

    if (cmd_count == 1) {
        if (builtin_command(args[0]))
            return;
        pid = Fork();
        if (pid == 0) {
            if (execvp(args[0][0], args[0]) < 0) {
                printf("%s: Command not found.\n", args[0][0]);
                exit(1);
            }
        }
        if (Waitpid(pid, &status, 0) < 0) {
            unix_error("waitpid error");
        }
        return;
    }

    for (int i = 0; i < cmd_count; i++) {
        if (i < cmd_count - 1) {
            if (pipe(p) < 0) {
                unix_error("pipe error");
            }
        }
        pid = Fork();
        if (pid == 0) {
            if (fd != STDIN_FILENO) {
                Dup2(fd, STDIN_FILENO);
                Close(fd);
            }
            if (i < cmd_count - 1) {
                Dup2(p[1], STDOUT_FILENO);
                Close(p[0]);
                Close(p[1]);
            }
            if (builtin_command(args[i])) {
                exit(0);
            }
            if (execvp(args[i][0], args[i]) < 0) {
                printf("%s: Command not found.\n", args[i][0]);
                exit(1);
            }
        }
        if (Waitpid(pid, &status, 0) < 0) {
            unix_error("waitpid error");
        }
        if (fd != STDIN_FILENO) {
            Close(fd);
        }
        if (i < cmd_count - 1) {
            Close(p[1]);
            fd = p[0];
        }
    }
}

void free_args(char ***args, int cmd_count) {
    if (!args)
        return;
    for (int i = 0; i < cmd_count; i++) {
        for (int j = 0; args[i][j]; j++) {
            Free(args[i][j]);
        }
        Free(args[i]);
    }
    Free(args);
}

int builtin_command(char **args) {
    if (!strcmp(args[0], "exit")) {
        exit(0);
    }
    if (!strcmp(args[0], "&")) {
        return 1;
    }
    if (!strcmp(args[0], "cd")) {
        if (args[1]) {
            if (chdir(args[1])) {
                printf("-bash: cd: %s: No such file or directory\n", args[1]);
            }
        } else {
            char *home = getenv("HOME");
            if (home == NULL || chdir(home)) {
                printf("-bash: cd: HOME not set or directory not found\n");
            }
        }
        return 1;
    }
    return 0;
}