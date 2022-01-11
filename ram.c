#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ram.h"
#include "memorymanager.h" //for 'isWhitespaceline'

//int nextFree = 0;

//debug**********************************************
void printram(){
  for (int i = 0 ; i < 40; i++){
    if(i %4==0){
      printf("---------------\n");
    }
    if(ram[i] ==NULL){
      printf("NULL\n");
    }else{
      printf("%s\n", ram[i]);
    }
  }
}

//**************************************************

void initRam(){
    for (int i = 0 ; i < RAM_SIZE; i++){
        ram[i] = NULL;
    }
}


/**
 * frees the elements of the ram array and 
 * sets the pointers of ram to NULL.
 * Make sure the pointers in the cells of 'ram'
 * point to an address returned from *alloc
 * (i.e. make sure you fill these lines by strdup-ing)
*/
void freeRam(int frameno){
    int ramIndex = frameno * FRAME_SIZE;
    int counter = 0;
    while(counter < FRAME_SIZE){ 
        free(ram[ramIndex+counter]);                     
        ram[ramIndex+counter] = NULL;
        counter++;
    }
}

void freeAllRam(){
    for(int i = 0 ; i < RAM_SIZE; i++){
        if(ram[i]!=NULL){
            free(ram[i]);
        }
    }
}

// No memory is assigned to the ram array in the case of only 
//whitespace lines. No need to free anything inthat case
void addToRAM( FILE *fp, int start, int end){
    char buffer[1000] = {'\0'};
    
    int counter = start;
    fgets(buffer, 999, fp);
    
    while (!feof(fp) && counter <= end){ // make sure start = frameno
        if(!isWhitespaceLine(buffer)){   // and end = frameno + FRAME_SIZE - 1
            ram[counter] = strdup(buffer);
            fgets(buffer, 999, fp);
            counter++;
        }else{
            fgets(buffer, 999, fp);
        }
    }
}
