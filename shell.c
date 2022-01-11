#include "interpreter.h"
#include "shellmemory.h"
#include "kernel.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int printPrompt = 0;

int shellUI()
{
    int errcode = 0;
    while (!feof(stdin))
    {   
        if(!isatty(stdin)||printPrompt){
            printf("$ ");
        }
        fflush(stdout);
        char *line = NULL;
        size_t linecap = 0;
        if (getline(&line, &linecap, stdin) == -1)
            break;
        errcode = interpret(line);
        free(line);
        if (feof(stdin))   
        {
            fclose(stdin);                      //doesn't work
            stdin=fopen("/dev/tty", "r");       //this either
            printPrompt = 1;
        }
    }
    return errcode;
}