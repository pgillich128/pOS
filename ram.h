#ifndef RAM_H
#define RAM_H

#include <stdlib.h>

#define RAM_SIZE 40
#define FRAME_SIZE 4

char* ram [RAM_SIZE];

void initRam();

void addToRAM();

void freeAllRam();
void freeRam(int frameno);

//debug
void printram();
#endif   