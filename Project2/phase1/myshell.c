#define _CRT_SECURE_NO_WARNINGS
#include "myshell.h"
#include <errno.h>


int main() {
    do {
        // Shell Prompt
        printf("CSE4100-SP-P2> ");

        // Reading
        char *input = myshell_readinput();
        if (input == NULL) { // NULL Check
            exit(0);
        }

        // Parsing
        char **args = myshell_parseinput(input);
        if (args[0] == NULL) { // NULL Check
            free(input);
            continue;
        }

        // Executing
        myshell_execute(args);

        // Free
        free(input);
        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }
        free(args);
    } while (1);
    return 0;
}

/* myshell_readinput : Read a line from stdin */
char *myshell_readinput(void) {
    char *input = Malloc(MAXLINE);
    if (fgets(input, MAXLINE, stdin) == NULL) {
        free(input);
        return NULL; // NULL Check
    }
    return input;
}

/* myshell_parseinput : Parse the input string to arguments */
char **myshell_parseinput(char *input) {
    char **args = Malloc(MAXARGS * sizeof(char *));
    char *buf = input;
    int argc = 0;

    buf[strlen(buf) - 1] = ' '; // Change '\n' to space
    while (*buf && (*buf == ' ')) // Skip spaces
        buf++;

    char *delim;
    while ((delim = strchr(buf, ' '))) {
        args[argc] = Malloc(delim - buf + 1);
        strncpy(args[argc], buf, delim - buf);
        args[argc][delim - buf] = '\0';
        argc++;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) // Skip spaces
            buf++;
    }
    args[argc] = NULL;
    return args;
}

/* myshell_execute - Execute given command */
void myshell_execute(char **args) {
    // Built-in commands
    if (!strcmp(args[0], "exit")) {
        exit(0);
    }
    if (!strcmp(args[0], "cd")) {
        char *dir = args[1] ? args[1] : getenv("HOME");
        if (chdir(dir) < 0) {
            printf("cd: %s: %s\n", dir, strerror(errno));
        }
        return;
    }

    // External commands
    pid_t pid = Fork();
    if (pid == 0) { // Child process
        if (execvp(args[0], args) < 0) {
            printf("%s: Command not found.\n", args[0]);
            exit(1); // Ensure the child process is terminated if fail
        }
    } else { // Parent process
        int status;
        if (Waitpid(pid, &status, 0) < 0) {
            unix_error("Waitpid error");
        }
    }
}