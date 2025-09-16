#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>
#define PROCESSNAME_SIZE 5

// Struct for save each new Process. When the process is been terminated we ll change its pid to 0 like a flag.
typedef struct Process
{
    pid_t pid;
    char processName[PROCESSNAME_SIZE];
    int semIndex;
    int acksemIndex;
    int StartedCycle;
} Process;

//Respective functions

void terminateAllProcesses(Process Processes[], int ActiveProcessCount);
void childProcess(int semid, int shmid, int semIndex, int ackSemIndex, const char *processName, int StartCycle); // the core functionality of each active child process
// Print the active processes in each cycle of P process for monitoring reasons .
void printProcesses(Process Processes[], int ActiveProcessCount);
#endif //PROCESS_H