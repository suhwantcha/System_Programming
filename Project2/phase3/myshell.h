#ifndef MYSHELL_H
#define MYSHELL_H

#include "csapp.h"

#define MAXARGS 128
#define MAXCMDS 10

typedef enum { FG, BG, STOP, DONE, TERM } job_state;

typedef struct job {
    pid_t pgid;
    job_state state;
    int jid;
    char *cmdline;
    volatile struct job *next;
    volatile struct job *prev;
} Job;

void init_jobs(void);
void add_job(pid_t pgid, job_state state, char *cmdline);
void delete_job(pid_t pgid);
void update_job_state(pid_t pgid, job_state state);
volatile Job *find_job_by_jid(int jid);
void print_jobs(void);
void clear_done_jobs(void);
void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);
char *myshell_readinput(void);
char ***myshell_parseinput(char *input, int *cmd_count, int *background);
void myshell_execute(char ***args, int cmd_count, int background, char *cmdline);
void free_args(char ***args, int cmd_count);
int builtin_command(char **args);

#endif