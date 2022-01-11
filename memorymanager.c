#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ram.h"
#include "kernel.h"
#include "pcb.h"
#include "memorymanager.h"


int isWhitespace(char c){
    switch(c){
		case ' ': 
			return 1;
			break;

		case '\t':
			return 1;
			break;
        case '\n':
            return 1;
            break;
        case '\r':
            return 1;
            break;
	}
    return 0;
}

int isWhitespaceLine(const char* line){
    for(int i = 0; i < strlen(line); i++){
        if(!isWhitespace(line[i])){
            return 0;
        }
    }
    return 1;
}

int countTotalPages(FILE* fp){
    //return total number of pages required by the program
    //Let L be the # of lines in the program
    // L <= 4 -> 1 page
    // 4 < L <= 8 -> 2 etc...
    //assumes that the file has no blank lines (BackingStore copy has no blank lines)
    int lnCount = 0;
    while (!feof(fp))
    {
        char *line = NULL;
        size_t linecap = 0;
        getline(&line, &linecap, fp);
        if(!isWhitespaceLine(line)){
            lnCount++;
        }
        free(line);
    }
    if(lnCount == 0){
        return 0;
    }
    
    if(lnCount % FRAME_SIZE ==0){
        return lnCount/FRAME_SIZE ;
    }
    return (int)(lnCount/FRAME_SIZE)+1;
}

void loadPage(int pageNumber, FILE *fp, int frameNumber)
{
    //FILE *fp points to the beginning of the file in the Backing Store.
    //The variable pageNumber is
    //the desired page from the Backing Store. The function loads the 4 lines of code from the page
    //into the frame in ram[].

    //skip to the requested page (page "pageNumber")
    size_t linecap = 0;
    int i = 0;
    while (i < pageNumber * FRAME_SIZE && !(feof(fp)))
    {
        char *discardLine = NULL;
        getline(&discardLine, &linecap, fp);    //read one line from the file
        free(discardLine);
        i++;
    }

    //now at the requested page in the file
    int start = frameNumber * FRAME_SIZE; 
    int end = start + FRAME_SIZE - 1;
    addToRAM(fp, start, end);
}

int findFrame()
{//there are RAM_SIZE/FRAME_SIZE frames in RAM
    int numFrames = RAM_SIZE/FRAME_SIZE;
    int freeframe = -1;
    for (int i = 0; i < numFrames; i++)
    {
        if(ram[i * FRAME_SIZE]==NULL){
            freeframe = i;
            return freeframe;
        }
    }
    return freeframe;
}

int getPageFromFrame(PCB *p, int frameno){
    // lookup page number from frame number in PCB page table
    int pageno = -1;
    int pgtblsize = sizeof(p->pageTable)/sizeof(int);
    for (int i = 0 ; i < pgtblsize; i++){
        if(p->pageTable[i]==frameno){
            return i;
        }
    }
    return pageno;
}

int findVictim (PCB* p){
    //only called if findFrame returns 1. 
    //choose frame number via random number generator
    // if frame number not already in use by the current PCB then return that frame
    // else iterate starting from this number until you find one. 
    int numframes = (RAM_SIZE/FRAME_SIZE);
    
    int randomFrame = rand() % numframes;
   
    if( frameBelongsTo(p, randomFrame) ){   //if the random frame belongs to the current pcb
        int candidateVictim = (randomFrame++) % numframes;  //increment to the next frame after random
        while((candidateVictim != randomFrame)&& //while we didn't make a complete loop
            (frameBelongsTo(p, candidateVictim)))            //and the new candidate victim still belongs to the pcb
        {
            candidateVictim = (candidateVictim ++) % (numframes);
        }
        return candidateVictim; 
    }
    
}

int updatePageTable(PCB* p, int pageNumber, int frameNumber, int victimFrame){
    // victimFrame = -1 then there is no victim, just update the page table entry for PCB p
    // otherwise, still update the page table for p, but also have to cycle through the
    // other active pcb's to figure out whose frame we stole
    // Since all active pcb's will be in the ready queue, we can cycle through the ready queue
    // until we find the process that we stole from. 

    //IMPORTANT: calling update page table with a valid victim automatically updates
    //the victim's page table (recursively)

    int errcode = 0;
    if(victimFrame == -1){      // if we didn't steal a frame from another PCB
        p->pageTable[pageNumber] = frameNumber;
    }else{
        // note we don't use the value frameNumber in this branch
        p->pageTable[pageNumber] = victimFrame; //steal someone else's frame
        PCB* victim = getFrameOwner(victimFrame);   // get the victim pcb

        //------------------------lookup the page belonging to frame-----update vic pgtbl entry invalid---didn't steal
        updatePageTable( victim, getPageFromFrame(victim, victimFrame),          -1,                          -1);
    }
    return 0;
}

char* makeBSfilename(char* sourcename, int pid){
    //can use strdup (malloc) to make the modified source file name (BSfilename).
    //This will fill one of the fields of a PCB, we'll free it when we kill the PCB
    //there won't be a double free issue, nor memory leak, since 
    //original filename's space is reclaimed in interpreter.c
    //and the pointer to the BSfilename points to different heap address
    //from the address of the original file name

    int numPIDdigits = snprintf(NULL, 0, "%d", pid);           //neat trick: returns the size that would have been copied--no need to calculate

    char digitsBuffer [numPIDdigits+1];                        // a buffer to contain the string representation of the pid
    snprintf(digitsBuffer, numPIDdigits+1, "%d", pid);         // store the digit representation of the pid in the digits buffer

    int BSsize =  strlen(sourcename) + numPIDdigits + 1 ;      
    
    char *BSfilename = (char*)calloc(BSsize, sizeof(char));
    strcat(BSfilename, sourcename);
    strcat (BSfilename, digitsBuffer);

    return BSfilename;
}

void copyFile( FILE *source, FILE *dest)
{
    //copy source to dest line by line
    size_t linecap = 0;
    while (!feof(source))
    {
        char *line = NULL;
        getline(&line, &linecap, source);
        if(!isWhitespaceLine(line)){ //only copy lines that contain text    
            fprintf(dest, "%s", line);
        }
        free(line);
    }
    fprintf(dest, "\n");
}

void loadPageAndUpdate(PCB* p, int pageno){
    // loads a missing page from process p into RAM and adds
    // p to the back of the ready queue after updating p's page table
    int frameno = findFrame();
    FILE *fp = fopen(p->filePath, "r");
    if(fp == NULL){
        printf("file %s not found in function memorymanager.pagefault(). Aborted.\n", p->filePath);
    }
    if(frameno != -1){  // if there was no victim (a free frame existed in RAM)
        loadPage(pageno, fp, frameno); // just loads the page into RAM
        updatePageTable(p, pageno, frameno, -1); // there was no victim
    }else{
        int victimframe = findVictim(p);
        freeRam(victimframe);
        loadPage(pageno, fp, victimframe);
        updatePageTable( p, pageno, -1, victimframe); // input of frameNumber argument not used if 
                                                      // there is a valid victim frame
                                                      // updatePageTable with valid victim also
                                                      // updates the victim PCB
    }
    fclose(fp);
}

int pageFault(PCB* p){
    // handles a page fault if the requested page of the PCB p is not in ram
    // if the program has terminated, then kill the process and free its ram
    // returns: 1 if process terminated, 0 otherwise

    //determine the next page
    (p->PC_page)++;
    int requestedPageno = p->PC_page; 
    int frameno = p->pageTable[requestedPageno];
    p->PC_offset = 0;

    if(requestedPageno >= p->pages_max){ // pages_max is the NUMBER of pages in the program
                                         // requestedPageno is an index
        //printf("Process %d terminated\n", p->ID);
        killProcess(p);
        return 1;
    }else{
        if(frameno != -1){ // page table entry is valid
            //printf("Process %d valid pgtbl entry\n", p->ID);
            p->PC = FRAME_SIZE * frameno;
        }else{
            //printf("Process %d updating page table\n", p->ID);        
            loadPageAndUpdate(p, requestedPageno); // load new page in ram and update page table
                                              // of p and of any victim of p
            frameno = p->pageTable[requestedPageno];
            p->PC = FRAME_SIZE * frameno;
        }
        return 0;
    }

}
int launcher (FILE* source, char *filename){
    //returns 1 on success, 0 on failure

    int errcode;
    int pid;
    char BSfilepath [1000];
    char* BSfilename;

    //assign a process ID to the PCB we will eventually create. we'll use the PID to modify the BackingStore file name
    pid = assignPID();

    //name the BackingStore version of the file as "originalNamePID"
    BSfilename = makeBSfilename(filename, pid);
    snprintf(BSfilepath, 1000, "./BackingStore/%s", BSfilename);
    
    //copy the file line by line 
    FILE *dest = fopen (BSfilepath, "wt");
    if(dest ==NULL){
        printf("problem creating file %s, operation aborted\n", BSfilename);
    }

    copyFile(source, dest);         //only copies the non blank lines
    fclose(dest);
    fclose(source);

    //count total number of pages in the (empty-line-free) backing store copy
    FILE *BScpy = fopen(BSfilepath, "r");
    if(BScpy ==NULL){
        printf("error reopening %s for counting num pages\n", BSfilename);
        return 1;
    }
    int numpages = countTotalPages(BScpy); 
    fclose (BScpy);
    if(numpages ==0){ // if the file had no instructions, just return
        free(BSfilename);  // free the filename, BSfilepath doesn't need to be freed
        BSfilename = NULL;
        return 0;
    }

    // now that we know how long the program is, create a PCB
    // then load each page into RAM (one or two pages)
    // updating the PCB's page table each time.

    //make pcb:
    PCB* pcb = makePCB(numpages, pid, BSfilepath );
    if (pcb == NULL){
        return 1; // happens if there was a problem with malloc or
                  // if the program in filename had only whitespace (no commands)
    }
        
    // load some number of pages into RAM
    BScpy = fopen(BSfilepath, "r"); 
    int pagesToLoad;
    if(numpages <= 1){
        pagesToLoad = 1;
    }else{
        pagesToLoad = 2;
    }
    for (int i = 0 ; i < pagesToLoad; i++){
        loadPageAndUpdate(pcb, i);
    }
    fclose(BScpy);
    //set the PC of the new PCB to point to the first frame corresponding to the first page
    //careful: pcb->PC points is the index in ram of current instruction, NOT the page number
    pcb->PC = (pcb->pageTable[0])*FRAME_SIZE;

    addToReady(pcb);

    return 0;

}