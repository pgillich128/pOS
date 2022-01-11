#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pcb.h"

PCB *makePCB(int pagesMax, int pid, char *filePath) 
{
   PCB *pcb = (PCB *)malloc(sizeof(PCB));
   if (pcb == NULL)
   {
      printf("Failed to create PCB, aborting command\n");
      return NULL;
   }
   pcb->PC = -1; //is the index of current page's frame
   pcb->ID = pid;
   pcb->PC_page = 0; // the current page of the program counter
   pcb->PC_offset = 0;  //the offset from start of page
   pcb->pages_max = pagesMax; //max number of pages
   pcb->filePath = strdup(filePath);  //the name of the file in the backing store
   
   int pgtblsz = sizeof(pcb->pageTable)/sizeof(int);

   for (int i = 0 ; i < pgtblsz; i++){
      pcb->pageTable[i] = -1;
   }

   return pcb;
}
