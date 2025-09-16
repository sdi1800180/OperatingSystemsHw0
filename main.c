#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int cycleCounter = 0;
    int txtlinescounter;
    if (argc != 4)
    {
        printf("Usage: %s <command_file> <text_file> <max_processes>\n", argv[0]);
        return 1;
    }
    
    //Arguments management
    const char *command_file = argv[1];
    const char *text_file = argv[2];
    int max_processes = atoi(argv[3]);
    
    // Parsing the files argument
    Command *commands = parseConfig(command_file);
    TextLine *textLines = readTextFile(text_file);
    txtlinescounter = textLines->linecounter;
    // printf("linesCounter is %d\n",txtlinescounter);
    // sleep(3);
    if (commands == NULL || textLines == NULL)
    {
        return 1;
    }
    //For debugging. If you want to see this put a sleep() after that 
    printCommands(commands);

    // Initialize all pids with zero such as there are no active child processes yet !
    Process Processes[max_processes];
    int ActiveProcessCount = 0;
    for (int i = 0; i < max_processes; i++)
    {
        Processes[i].pid = 0;
    }
    
    // Create semaphore array . 2 semaphores for each child process.
    // The second is for declaration of aknowoledge to parent process 
    // that the message in the shared memory has been read
    int semid = semget(IPC_PRIVATE, max_processes * 2, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget");
        return 1;
    }
    
    // Initialize semaphores => 2 semaphores per active child
    for (int i = 0; i < max_processes * 2; i++)
    {
        if (semctl(semid, i, SETVAL, 0) == -1)
        {
            perror("semctl");
            return 1;
        }
    }
    
    // Create shared memory segment. "SharedMemory" struct data type.
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shmget");
        return 1;
    }
    
    // Seed the random number generator
    srand(time(NULL));

    Command *currentCommand = commands;
    // Now the "life" of parent process begins . (The loop actually)
    while (1)
    {
        cycleCounter++;

        printf("I am P parent process with my pid = %d and my cycle is %d -----------------------------------------------------------------------------------------\n", getpid(), cycleCounter);
        int actionFound = 0;
        // Before all the actions, print the active processes for monitor the current sate. 
        printProcesses(Processes, max_processes);
        
        if (currentCommand != NULL && currentCommand->timestamp == cycleCounter)
        {
            printf("%d - %s - %s\n", currentCommand->timestamp, currentCommand->processName, currentCommand->command);
            // actionFound is used for flag in order to ensure that the P process found a command at the current timestamp
            actionFound = 1;
            int res = processCommand(currentCommand,max_processes, Processes, &ActiveProcessCount, cycleCounter, semid, shmid);
            // if processCommand function return 0 it means that read the "EXIT".
            
            if (!res)
            {
                printf("I am P process and I am ending. But first I will terminate all my remaining children...\n");
                terminateAllProcesses(Processes, max_processes);
                //First terminate all the proceesses, after that cleaning up semaphores and shared memory
                if (semctl(semid, 0, IPC_RMID) == -1)
                {
                    perror("semctl");
                }
                if (shmctl(shmid, IPC_RMID, NULL) == -1)
                {
                    perror("shmctl");
                }

                
                Command *current = commands;
                while (current != NULL)
                {
                    Command *next = current->next;
                    free(current->processName);
                    free(current->command);
                    free(current);
                    current = next;
                }
                
                freeTextLines(textLines);
                //Exit the programm !!
                return 0;
            }
            currentCommand = currentCommand->next;
        }
        // if actionFound is zero it means that no command found for this timestamp
        if (!actionFound)
        {
            printf("There are no commands in this cycle %d\n", cycleCounter);
        }
        // After all that , P process must select a random child running process , and a random text line to send it there.
        sendTextToChild(semid, shmid, textLines, txtlinescounter, Processes, ActiveProcessCount, max_processes);
    }
}