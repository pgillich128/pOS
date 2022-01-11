#ifndef MEM_MAN_H
#define MEM_MAN_H
#include <stdio.h>
#include "pcb.h"

int isWhitespaceLine(const char* line);
void loadPageAndUpdate(PCB* p, int pageno);
int pageFault(PCB* p);
int launcher (FILE *fp, char* filename);

#endif