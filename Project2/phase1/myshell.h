#ifndef MYSHELL_H
#define MYSHELL_H

#include "csapp.h"

#define MAXARGS 128

char *myshell_readinput(void);
char **myshell_parseinput(char *input);
void myshell_execute(char **args);

#endif /* MYSHELL_H */