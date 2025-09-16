#ifndef MYSHELL_H
#define MYSHELL_H

#include "csapp.h"

#define MAXARGS 128
#define MAXCMDS 16

char *myshell_readinput(void);
char ***myshell_parseinput(char *input, int *cmd_count);
void myshell_execute(char ***args, int cmd_count);
void free_args(char ***args, int cmd_count);

void eval(char* cmdline);
int parseline(char* buf, char** argv);
int builtin_command(char** argv);
void operate_pipe(char*** cmd, int cmd_count, int cmd_idx, int input_fd);
void operate_one(char** cmd);
char*** set_command(char** argv, int *cmd_count);
void free_command(char*** cmd, int cmd_count);

#endif /* MYSHELL_H */