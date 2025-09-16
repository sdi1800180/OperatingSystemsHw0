#ifndef COMMANDS_H
#define COMMANDS_H

#include "process.h"
#include "text.h"
#include "sharedmem.h"


// Struct for save each line of configuration file
typedef struct Command
{
    int timestamp;
    char *processName;
    char *command;
    struct Command *next;
} Command;


// Respective functions 
Command *parseConfig(const char *filename);
void printCommands(Command *head);
int processCommand(Command *command, int maxProcesses, Process Processes[], int *ActiveProcessCount, int cycleCounter, int semid, int shmid);
void sendTextToChild(int semid, int shmid, TextLine *textLines,int lines, Process Processes[], int ActiveProcessCount, int max_processes);

#endif //COMMANDS_H