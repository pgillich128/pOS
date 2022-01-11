#ifndef KERNEL_H
#define KERNEL_H
#include "pcb.h"

#define QUANTA 2

extern int numProcesses;

typedef struct rqnode{
    PCB* pcb;
    struct rqnode* next;
}rqnode;

typedef struct rq{
    rqnode* head;
    rqnode* tail;
}rq;

int quit_state;
int initial_quit_state; // variables that indicate if we want to quit or not
void quitCleanup(); // free various pointers when it's time to quit
int scheduler();    // manages the ready queue and processing of PCB's
void addToReady(PCB* pcb);  //add a pcb to the back of the ready queue
int initRQ();   //inititalize the ready queue
void freeRQ();  // free the ready queue
int assignPID(); // assigns a PID to a new PCB
PCB* getFrameOwner(int frameno);    // returns the PCB of the process that occupies the frame frameno
int frameBelongsTo(PCB *p, int frameno);
void taskSwitch();
void killProcess(PCB* p);
#endif
