#ifndef SHAREDMEM_H
#define SHAREDMEM_H

#define SHM_SIZE 1024


//Struct for Share memory. Forced to use struct data type, 
//cause we need a int member in order to pass to child process 
//the current cycle of P process that is the final for child.

typedef struct SharedMemory
{
    char message[SHM_SIZE];
    int FinalCycle;
} SharedMemory;


#endif //SHAREDMEM_H