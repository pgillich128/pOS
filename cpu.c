#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "ram.h"
#include "interpreter.h"

CPU* cpu; 

int getCPUIP(){
    return cpu->IP;
}

int getCPUOffset(){
    return cpu->offset;
}

int intitialize_cpu(){
    cpu = (CPU*)malloc(sizeof(CPU));

    
    if(cpu ==NULL){
        printf("Failed to load CPU...bummer\n");
        return 1;
    }

    cpu->IP = 0; 
    cpu->offset = 0;
    cpu->quanta = 0;
    
    //set IR[] to all '\0'
    for (int i = 0 ; i < 1000; i++){
        cpu->IR[i] = '\0';
    }

    return 0;
}

int cpuReady(){
    return (cpu->quanta == 0);
}

/**
 * runCPU assumes FIRST isn't NULL, but it won't be
 * because this function is only called when FIRST
 * is a real PCB
 * 
 * */
int runCPU(int quanta)
{
    //printf("IN CPU: %d, %s\n", FIRST->ID, FIRST->filePath);


    // return values: 0: regular context switch
    //                1: page fault, then task switch
    cpu->quanta = quanta;
    cpu->IP = FIRST->PC; //PC: index in RAM of current page (i.e. the frame)
    cpu->offset = FIRST->PC_offset;
    while (cpu->quanta > 0)
    {
        if (cpu->offset >= FRAME_SIZE) //page fault
        {
            //printf("page fault P%d\n", FIRST->ID);
            cpu->quanta = 0; //generates pseudo-interrupt
            return 1;
        }

        int nextInstrIndex = cpu->IP + cpu->offset;
        if (ram[nextInstrIndex] != NULL)
        {
            strncpy(cpu->IR, ram[nextInstrIndex], sizeof(cpu->IR) - 1);
            (void)interpret(cpu->IR);
        }

        (cpu->offset)++;
        cpu->quanta--;
    }

    cpu->quanta = 0;     //reset the quanta to 0 signalling cpu now available
    return 0;
}

void freeCPU()
{
    free(cpu);
}