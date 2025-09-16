#define _CRT_SECURE_NO_WARNINGS
#include "myshell.h"

volatile Job *jobs = NULL;
volatile static int next_jid = 1;
static const char* STATE_STR[] = { "Foreground", "Running", "Stopped", "Done", "Terminated" };
volatile static pid_t shell_pgid;

void init_jobs(void) {
    jobs = Malloc(sizeof(Job));
    jobs->pgid = 0;
    jobs->state = FG;
    jobs->jid = 0;
    jobs->cmdline = NULL;
    jobs->next = NULL;
    jobs->prev = NULL;
    next_jid = 1;
    shell_pgid = getpid();
}

void add_job(pid_t pgid, job_state state, char *cmdline) {
    sigset_t mask, prev;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    Sigprocmask(SIG_BLOCK, &mask, &prev);

    volatile Job *job = Malloc(sizeof(Job));
    job->pgid = pgid;
    job->state = state;
    job->jid = next_jid++;
    job->cmdline = strdup(cmdline);
    job->next = NULL;
    job->prev = jobs;

    if (jobs->next) {
        jobs->next->prev = job;
        job->next = jobs->next;
    }
    jobs->next = job;

    Sigprocmask(SIG_SETMASK, &prev, NULL);

    if (state == BG) {
        char buf[256];
        snprintf(buf, 256, "[%d] (%d) %s\n", job->jid, pgid, cmdline);
        Rio_writen(STDOUT_FILENO, buf, strlen(buf));
    }
}

void delete_job(pid_t pgid) {
    sigset_t mask, prev;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    Sigprocmask(SIG_BLOCK, &mask, &prev);

    volatile Job *job = jobs->next;
    while (job) {
        if (job->pgid == pgid) {
            if (job->prev) job->prev->next = job->next;
            if (job->next) job->next->prev = job->prev;
            Free(job->cmdline);
            Free((void *)job);
            break;
        }
        job = job->next;
    }

    Sigprocmask(SIG_SETMASK, &prev, NULL);
}

void update_job_state(pid_t pgid, job_state state) {
    sigset_t mask, prev;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    Sigprocmask(SIG_BLOCK, &mask, &prev);

    volatile Job *job = jobs->next;
    while (job) {
        if (job->pgid == pgid) {
            job->state = state;
            break;
        }
        job = job->next;
    }

    Sigprocmask(SIG_SETMASK, &prev, NULL);
}

volatile Job *find_job_by_jid(int jid) {
    volatile Job *job = jobs->next;
    while (job) {
        if (job->jid == jid) return job;
        job = job->next;
    }
    return NULL;
}

void print_jobs(void) {
    clear_done_jobs();
    volatile Job *job = jobs->next;
    int count = 0;
    volatile Job *last_job = NULL;

    while (job) {
        if (job->state == STOP || job->state == BG) {
            count++;
            last_job = job;
        }
        job = job->next;
    }

    job = jobs->next;
    while (job) {
        if (job->state == BG || job->state == STOP) {
            char indicator = (job == last_job ? '+' : (job->next == last_job && count <= 2 ? '-' : ' '));
            char buf[256];
            snprintf(buf, 256, "[%d]%c  %s %s\n", job->jid, indicator, STATE_STR[job->state], job->cmdline);
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
        }
        job = job->next;
    }
}

void clear_done_jobs(void) {
    sigset_t mask, prev;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    Sigprocmask(SIG_BLOCK, &mask, &prev);

    volatile Job *job = jobs->next;
    while (job) {
        if (job->state == DONE || job->state == TERM) {
            char indicator = (job->next == NULL ? '+' : (job->next->next == NULL ? '-' : ' '));
            char buf[256];
            snprintf(buf, 256, "[%d]%c  %s %s\n", job->jid, indicator,
                     job->state == DONE ? "Done" : "Terminated", job->cmdline);
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            volatile Job *temp = job;
            job = job->next;
            if (temp->prev) temp->prev->next = temp->next;
            if (temp->next) temp->next->prev = temp->prev;
            Free(temp->cmdline);
            Free((void *)temp);
        } else {
            job = job->next;
        }
    }

    Sigprocmask(SIG_SETMASK, &prev, NULL);
}

void sigchld_handler(int sig) {
    int old_errno = errno;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        pid_t pgid = getpgid(pid);
        volatile Job *job = jobs->next;
        while (job && job->pgid != pgid) job = job->next;
        if (!job) continue;

        if (WIFEXITED(status)) {
            update_job_state(job->pgid, DONE);
            delete_job(job->pgid);
            char buf[256];
            snprintf(buf, 256, "[%d] Done %s\n", job->jid, job->cmdline);
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
        } else if (WIFSIGNALED(status)) {
            update_job_state(job->pgid, TERM);
            delete_job(job->pgid);
            char buf[256];
            snprintf(buf, 256, "[%d] Terminated %s\n", job->jid, job->cmdline);
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
        } else if (WIFSTOPPED(status)) {
            update_job_state(job->pgid, STOP);
            char buf[256];
            snprintf(buf, 256, "[%d] Stopped %s\n", job->jid, job->cmdline);
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
        } else if (WIFCONTINUED(status)) {
            update_job_state(job->pgid, BG);
        }
    }
    errno = old_errno;
}

void sigtstp_handler(int sig) {
    int old_errno = errno;
    volatile Job *job = jobs->next;
    while (job) {
        if (job->state == FG) {
            Kill(-job->pgid, SIGTSTP);
            break;
        }
        job = job->next;
    }
    errno = old_errno;
}

void sigint_handler(int sig) {
    int old_errno = errno;
    volatile Job *job = jobs->next;
    while (job) {
        if (job->state == FG) {
            Kill(-job->pgid, SIGINT);
            break;
        }
        job = job->next;
    }
    errno = old_errno;
}

char *myshell_readinput(void) {
    char *input = Malloc(MAXLINE);
    if (tcgetpgrp(STDIN_FILENO) != shell_pgid) {
        if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
            char buf[256];
            snprintf(buf, 256, "tcsetpgrp: %s\n", strerror(errno));
            Rio_writen(STDERR_FILENO, buf, strlen(buf));
        }
    }
    if (Fgets(input, MAXLINE, stdin) == NULL) {
        Free(input);
        return NULL;
    }
    return input;
}

char ***myshell_parseinput(char *input, int *cmd_count, int *background) {
    char ***args = Malloc(MAXCMDS * sizeof(char **));
    for (int i = 0; i < MAXCMDS; i++) {
        args[i] = Malloc(MAXARGS * sizeof(char *));
        for (int j = 0; j < MAXARGS; j++) args[i][j] = NULL;
    }

    *background = 0;
    *cmd_count = 0;
    char *buf = input;
    while (*buf && *buf == ' ') buf++;

    int cmd_idx = 0, argc = 0;
    char *token_start = buf;
    int in_quotes = 0;

    while (*buf && cmd_idx < MAXCMDS) {
        if (*buf == '"') {
            if (in_quotes) {
                in_quotes = 0;
                if (buf > token_start) {
                    args[cmd_idx][argc] = Malloc(buf - token_start + 1);
                    strncpy(args[cmd_idx][argc], token_start, buf - token_start);
                    args[cmd_idx][argc][buf - token_start] = '\0';
                    argc++;
                }
                buf++;
                while (*buf == ' ') buf++;
                token_start = buf;
                continue;
            } else {
                in_quotes = 1;
                buf++;
                token_start = buf;
                continue;
            }
        }

        if (!in_quotes && (*buf == ' ' || *buf == '|' || *buf == '&' || *buf == '\n')) {
            if (buf > token_start && *token_start != ' ') {
                args[cmd_idx][argc] = Malloc(buf - token_start + 1);
                strncpy(args[cmd_idx][argc], token_start, buf - token_start);
                args[cmd_idx][argc][buf - token_start] = '\0';
                argc++;
            }

            if (*buf == '|') {
                if (argc > 0) {
                    args[cmd_idx][argc] = NULL;
                    cmd_idx++;
                    argc = 0;
                }
            } else if (*buf == '&') {
                *background = 1;
                args[cmd_idx][argc] = NULL;
                if (argc > 0) *cmd_count = cmd_idx + 1;
                break;
            }
            buf++;
            while (*buf == ' ') buf++;
            token_start = buf;
            continue;
        }
        buf++;
    }

    if (buf > token_start && cmd_idx < MAXCMDS && !*background && *token_start != ' ') {
        char *token_end = buf;
        while (token_end > token_start && (*(token_end - 1) == ' ' || *(token_end - 1) == '\n')) token_end--;
        if (token_end > token_start) {
            if (*(token_end - 1) == '&' && !in_quotes) {
                *background = 1;
                token_end--;
                while (token_end > token_start && *(token_end - 1) == ' ') token_end--;
            }
            if (token_end > token_start) {
                args[cmd_idx][argc] = Malloc(token_end - token_start + 1);
                strncpy(args[cmd_idx][argc], token_start, token_end - token_start);
                args[cmd_idx][argc][token_end - token_start] = '\0';
                argc++;
            }
        }
    }

    if (argc > 0) {
        args[cmd_idx][argc] = NULL;
        *cmd_count = cmd_idx + 1;
    }

    return args;
}

void myshell_execute(char ***args, int cmd_count, int background, char *cmdline) {
    if (cmd_count == 0) return;
    if (builtin_command(args[0])) return;

    int p[2], fd_in = STDIN_FILENO;
    pid_t pgid = 0;
    pid_t pids[MAXCMDS];
    int pid_count = 0;

    char *copyline = strdup(cmdline);
    if (copyline[strlen(copyline) - 1] == '\n') copyline[strlen(copyline) - 1] = '\0';
    if (background && copyline[strlen(copyline) - 1] != '&') strcat(copyline, " &");

    for (int i = 0; i < cmd_count; i++) {
        if (i < cmd_count - 1) {
            if (pipe(p) < 0) {
                char buf[256];
                snprintf(buf, 256, "pipe: %s\n", strerror(errno));
                Rio_writen(STDERR_FILENO, buf, strlen(buf));
            }
        }

        pid_t pid = Fork();
        if (pid == 0) {
            if (pgid == 0) pgid = getpid();
            Setpgid(0, pgid);
            if (fd_in != STDIN_FILENO) {
                Dup2(fd_in, STDIN_FILENO);
                Close(fd_in);
            }
            if (i < cmd_count - 1) {
                Close(p[0]);
                Dup2(p[1], STDOUT_FILENO);
                Close(p[1]);
            }
            Signal(SIGINT, SIG_DFL);
            Signal(SIGTSTP, SIG_DFL);
            if (execvp(args[i][0], args[i]) < 0) {
                char buf[256];
                snprintf(buf, 256, "%s: %s\n", args[i][0], strerror(errno));
                Rio_writen(STDERR_FILENO, buf, strlen(buf));
                exit(1);
            }
        }

        pids[pid_count++] = pid;
        if (pgid == 0) pgid = pid;
        Setpgid(pid, pgid);

        if (fd_in != STDIN_FILENO) Close(fd_in);
        if (i < cmd_count - 1) {
            Close(p[1]);
            fd_in = p[0];
        }
    }

    add_job(pgid, background ? BG : FG, copyline);

    if (!background) {
        tcsetpgrp(STDIN_FILENO, pgid);
        int status;
        pid_t wpid;
        while ((wpid = waitpid(-pgid, &status, WUNTRACED)) > 0) {
            if (WIFSTOPPED(status)) {
                update_job_state(pgid, STOP);
                char buf[256];
                snprintf(buf, 256, "[%d] Stopped %s\n", find_job_by_jid(next_jid-1)->jid, copyline);
                Rio_writen(STDOUT_FILENO, buf, strlen(buf));
                break;
            } else if (WIFEXITED(status)) {
                update_job_state(pgid, DONE);
                delete_job(pgid);
                break;
            } else if (WIFSIGNALED(status)) {
                update_job_state(pgid, TERM);
                delete_job(pgid);
                break;
            }
        }
        tcsetpgrp(STDIN_FILENO, shell_pgid);
    } else {
        int status;
        pid_t wpid;
        while ((wpid = waitpid(-pgid, &status, WNOHANG)) > 0) {
            if (WIFEXITED(status)) {
                update_job_state(pgid, DONE);
                delete_job(pgid);
                char buf[256];
                snprintf(buf, 256, "[%d] Done %s\n", find_job_by_jid(next_jid-1)->jid, copyline);
                Rio_writen(STDOUT_FILENO, buf, strlen(buf));
                break;
            } else if (WIFSIGNALED(status)) {
                update_job_state(pgid, TERM);
                delete_job(pgid);
                break;
            }
        }
    }
    Free(copyline);
}

void free_args(char ***args, int cmd_count) {
    for (int i = 0; i < cmd_count; i++) {
        for (int j = 0; args[i][j]; j++) Free(args[i][j]);
        Free(args[i]);
    }
    Free(args);
}

int builtin_command(char **args) {
    if (!strcmp(args[0], "exit")) {
        sigset_t mask, prev;
        Sigemptyset(&mask);
        Sigaddset(&mask, SIGCHLD);
        Sigprocmask(SIG_BLOCK, &mask, &prev);

        volatile Job *job = jobs->next;
        while (job) {
            volatile Job *next = job->next;
            if (job->state != DONE && job->state != TERM) {
                if (kill(-job->pgid, SIGTERM) < 0 && errno != ESRCH) {
                    char buf[256];
                    snprintf(buf, 256, "kill: %s\n", strerror(errno));
                    Rio_writen(STDERR_FILENO, buf, strlen(buf));
                } else {
                    char buf[256];
                    snprintf(buf, 256, "[%d] Terminated %s\n", job->jid, job->cmdline);
                    Rio_writen(STDOUT_FILENO, buf, strlen(buf));
                    update_job_state(job->pgid, TERM);
                    delete_job(job->pgid);
                }
            }
            job = next;
        }
        Sigprocmask(SIG_SETMASK, &prev, NULL);

        tcsetpgrp(STDIN_FILENO, shell_pgid);
        exit(0);
    }

    if (!strcmp(args[0], "cd")) {
        if (args[1]) {
            if (chdir(args[1])) {
                char buf[256];
                snprintf(buf, 256, "cd: %s: %s\n", args[1], strerror(errno));
                Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            }
        } else {
            char *home = getenv("HOME");
            if (home && chdir(home)) {
                char buf[256];
                snprintf(buf, 256, "cd: %s\n", strerror(errno));
                Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            }
        }
        return 1;
    }

    if (!strcmp(args[0], "jobs")) {
        print_jobs();
        return 1;
    }

    if (!strcmp(args[0], "bg")) {
        if (!args[1] || args[1][0] != '%') {
            char buf[256];
            snprintf(buf, 256, "bg: job id missing\n");
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            return 1;
        }
        int jid = atoi(args[1] + 1);
        volatile Job *job = find_job_by_jid(jid);
        if (!job) {
            char buf[256];
            snprintf(buf, 256, "bg: %%%d: no such job\n", jid);
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            return 1;
        }
        if (job->state == BG) {
            char buf[256];
            snprintf(buf, 256, "bg: job %d already in background\n", jid);
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            return 1;
        }
        job->state = BG;
        if (strchr(job->cmdline, '&') == NULL) {
            char *new_cmd = Malloc(strlen(job->cmdline) + 3);
            strcpy(new_cmd, job->cmdline);
            strcat(new_cmd, " &");
            Free(job->cmdline);
            job->cmdline = new_cmd;
        }
        Kill(-job->pgid, SIGCONT);
        char buf[256];
        snprintf(buf, 256, "[%d] %s\n", job->jid, job->cmdline);
        Rio_writen(STDOUT_FILENO, buf, strlen(buf));
        return 1;
    }

    if (!strcmp(args[0], "fg")) {
        if (!args[1] || args[1][0] != '%') {
            char buf[256];
            snprintf(buf, 256, "fg: job id missing\n");
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            return 1;
        }
        int jid = atoi(args[1] + 1);
        volatile Job *job = find_job_by_jid(jid);
        if (!job) {
            char buf[256];
            snprintf(buf, 256, "fg: %%%d: no such job\n", jid);
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            return 1;
        }
        job->state = FG;
        if (strchr(job->cmdline, '&')) {
            char *new_cmd = strdup(job->cmdline);
            new_cmd[strlen(new_cmd) - 2] = '\0';
            Free(job->cmdline);
            job->cmdline = new_cmd;
        }
        Kill(-job->pgid, SIGCONT);
        tcsetpgrp(STDIN_FILENO, job->pgid);
        pid_t pid;
        int status;
        while ((pid = waitpid(-job->pgid, &status, WUNTRACED)) > 0) {
            if (WIFSTOPPED(status)) {
                update_job_state(job->pgid, STOP);
                char buf[256];
                snprintf(buf, 256, "[%d] Stopped %s\n", job->jid, job->cmdline);
                Rio_writen(STDOUT_FILENO, buf, strlen(buf));
                break;
            } else if (WIFEXITED(status)) {
                update_job_state(job->pgid, DONE);
                delete_job(job->pgid);
                break;
            } else if (WIFSIGNALED(status)) {
                update_job_state(job->pgid, TERM);
                delete_job(job->pgid);
                break;
            }
        }
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        return 1;
    }

    if (!strcmp(args[0], "kill")) {
        if (!args[1] || args[1][0] != '%') {
            char buf[256];
            snprintf(buf, 256, "kill: job id missing\n");
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            return 1;
        }
        int jid = atoi(args[1] + 1);
        volatile Job *job = find_job_by_jid(jid);
        if (!job) {
            char buf[256];
            snprintf(buf, 256, "kill: %%%d: no such job\n", jid);
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            return 1;
        }
        if (job->state == DONE || job->state == TERM) {
            char buf[256];
            snprintf(buf, 256, "kill: job %d already terminated\n", jid);
            Rio_writen(STDOUT_FILENO, buf, strlen(buf));
            return 1;
        }
        job->state = TERM;
        Kill(-job->pgid, SIGTERM);
        char buf[256];
        snprintf(buf, 256, "[%d] Terminated %s\n", job->jid, job->cmdline);
        Rio_writen(STDOUT_FILENO, buf, strlen(buf));
        delete_job(job->pgid);
        return 1;
    }

    return 0;
}

int main() {
    init_jobs();
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    Signal(SIGTSTP, sigtstp_handler);
    Signal(SIGTTIN, SIG_IGN);
    Signal(SIGTTOU, SIG_IGN);

    Setpgid(0, 0);
    tcsetpgrp(STDIN_FILENO, shell_pgid);

    while (1) {
        clear_done_jobs();
        Sio_puts("CSE4100-SP-P2> ");
        char *input = myshell_readinput();
        if (!input) continue;

        int cmd_count, background;
        char ***args = myshell_parseinput(input, &cmd_count, &background);
        if (cmd_count == 0) {
            free_args(args, cmd_count);
            Free(input);
            continue;
        }

        myshell_execute(args, cmd_count, background, input);
        free_args(args, cmd_count);
        Free(input);
    }
    return 0;
}