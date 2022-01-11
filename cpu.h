#ifndef CPU_H
#define CPU_H

#include "pcb.h"

typedef struct CPU{
    int IP;
    int offset;
    char IR [1000];
    int quanta;
}CPU;

int getCPUIP(); // returns the instruction pointer of the cpu (called when task switching)

int getCPUOffset(); // returns the CPU page offset (called when task switching)

int cpuReady();

int runCPU(int quanta); // returns 0 for reg. task switch, 1 for page fault

int intitialize_cpu();

void freeCPU();  

#endif 