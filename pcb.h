#ifndef PCB_H
#define PCB_H

typedef struct PCB{
    int PC; //pointer to the current position in the process 
    int ID;
    int pageTable [10];
    int PC_page;
    int PC_offset;
    int pages_max;
    char *filePath;
}PCB;
     
PCB* FIRST; // the pcb that interacts with the CPU. Referenced in cpu.run 
            //and kernel.scheduler

PCB* makePCB( int pagesMax, int pid, char *filePath);
#endif   