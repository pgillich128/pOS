#include "shellmemory.h"
#include "shell.h"
#include "ram.h"
#include "cpu.h"
#include "pcb.h"
#include "kernel.h"
#include "memorymanager.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


rq* readyq;
int numProcesses = 0; // the number of processes created since boot time
rqnode *iterPtr;      // points to ready queue nodes, used for iterating over the ready queue

// //debug******************
// void printrq(){
//     int counter = 0;
//     rqnode* node = readyq->head;
//     while(counter < 3){
//         PCB* pcb = (node->pcb);
//         printf("PC: %d, start: %d, end %d \n", pcb->PC, pcb->start, pcb->end );
//         node = node ->next;
//         counter++;
//     }
// }

// //**************************


int initRQ(){
    readyq = (rq*)malloc(sizeof(rq));
    if (readyq ==NULL)return 1;
    readyq->head = NULL;
    readyq->tail = NULL;
    return 0;
}

void freeRQ(rq *readyQ){
    free(readyQ);
}

void freeRQNode(rqnode *node){
    free(node);
}

PCB* removeFirst(){ // returns null if ready queue is empty
    if(readyq->head != NULL){
        PCB* pcb;
        rqnode* first = readyq->head;           
        if(first->next == first){   //if there is only one element in readyq
            pcb = first->pcb;
            readyq->head = NULL;
            readyq->tail = NULL;
        }else{
            readyq -> head = (readyq->head)->next;    //set the head to point 
            (readyq->tail)->next = readyq->head;
            pcb = first->pcb;                         //take the pcb out of its node
        }
        freeRQNode(first);                            //free the node it came from 
        return pcb;
    }
    return NULL;
}

int assignPID(){
    return numProcesses;
}


int frameBelongsTo(PCB *p, int frameno){
    //decides if a frame belongs to the current PCB as per the page table
    //1: frame belongs to p
    //0: frame doesn't belong to p
    int isMine = 0;
    int pgtblSize = sizeof(p->pageTable)/sizeof(int);
    for (int i = 0; i < pgtblSize; i++){
        if(p->pageTable[i] == frameno){
            isMine = 1;
        }
    }
    return isMine;
}

/**
 * returns the PCB of the owner of frame frameno
 * cycles through the ready queue
 **/
PCB* getFrameOwner(int frameno)
{
    PCB *p = NULL;
    if (readyq->head != NULL)
    {
        iterPtr = readyq->head;
        do
        {
            p = iterPtr->pcb;
            if (frameBelongsTo(p, frameno))
            {
                break;
            }
            else
            {
                iterPtr = iterPtr->next;
            }
        } while (iterPtr != readyq->head);
    }
    // will never return null since this function is only called when 
    // we had to steal a frame from another process, which
    // would have to be in the ready queue
    return p;
}

void addToReady(PCB* pcb){
    //make a new readyqnode
    rqnode* node = (rqnode*)malloc(sizeof(rqnode));
    node->pcb = pcb;
    if(readyq->head ==NULL){ //if the ready queue is empty
        readyq->head = node;
        readyq->tail = node;
        node->next = node; 
    } else{
        (readyq->tail)->next = node;
        node->next = readyq->head;
        readyq->tail = node;
    }
}

void killPCB(PCB* p){
    // TODO: free any PCB fields that are malloc'd (i.e.)
    // the filename of the corresponding backing store file
    // and the PCB itself
    free(p->filePath);
    p->filePath = NULL;
    free(p);
    p = NULL;
}

void killProcess(PCB *p){
    //for each valid frame in the  p's page table, remove the frame from RAM
    //and then free the PCB and all associated malloc'd pointers
    int numPgTblEntries = sizeof(p->pageTable)/sizeof(int);
    for(int i = 0 ; i < numPgTblEntries; i++){ //free only valid page table entries
        if(p->pageTable[i]!= -1){
            freeRam(p->pageTable[i]);
        }
    }
    killPCB(p);
    p = NULL;
}

void taskSwitch(){
    //save necessary registers in PCB
    //and add the updated PCB to back of ready queue
    FIRST->PC = getCPUIP();             
    FIRST->PC_offset = getCPUOffset(); 
    addToReady(FIRST);
    FIRST = removeFirst();
}
/**
 * assumes there is at least one non-null pcb in the ready queue
 * this function is only called when a non-null pcb exists, so 
 * the assumption is guaranteed to be true
 * */
int scheduler(){
    int errcode = 0;
    int result;
    FIRST = removeFirst();
    while (!(FIRST ==NULL))
    {
        if (cpuReady())
        {
            result = runCPU(QUANTA);
            if (result == 0)
            {
                taskSwitch();
            }
            else
            {
                int terminated = pageFault(FIRST);
                if (!terminated)
                {
                    addToReady(FIRST);
                    FIRST = removeFirst();
                    continue;
                }else{
                    FIRST = removeFirst();
                }
            }
        }
    }
    return errcode;
}


void quitCleanup(){
    shell_memory_destory();
    while(readyq->head !=NULL){     //frees the pcb's in the ready queue as well as the nodes
        PCB* pcb = removeFirst();
        free(pcb);
    }
    freeRQ(readyq);
    freeAllRam();
    freeCPU();
}
  
int kernel(){
    int errcode;
    (void)initRQ();
    shell_memory_initialize();
    initial_quit_state = 0;
    printf( "Kernel 1.0 loaded!\n"
            "Welcome to the Peter shell!\n"
           "Version 3.0 Updated March 2021\n");
    
    errcode = shellUI();  
    return errcode;
}

int initBackingStore(){
    int errcode;
    //delete the backing store if it exists already
    const char* delete  = "if [ -d BackingStore ]; then rm -Rf BackingStore; fi";
    errcode = system(delete);
    //make a new backing store
    if(errcode!=0){
        printf("System command failed. Aborting.");
        exit(errcode);
    }
    const char* newBS = "mkdir BackingStore";
    errcode = system(newBS);
    if(errcode!=0){
        printf("System command failed. Aborting.");
        exit(errcode);
    }
    return errcode;
}

int boot(){ // inititalizes ram and CPU
    int errcode;
    errcode = intitialize_cpu(); // if errcode = 1 failure, errcode = 0 good
    if(errcode ==1) return errcode;
    initRam();
    errcode = initBackingStore();
    return errcode;
}

int main(int argc, const char *argv[]){
    int error = 0;
    boot(); // inititlize the CPU (weird, I know)
            // as well as inititalizing ram
    error = kernel();
    return error;
}