#include "process.h"
#include "sharedmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>


// Function for each child process that running. We use two semaphores for each process.
void childProcess(int semid, int shmid, int semIndex, int ackSemIndex, const char *processName, int StartCycle)
{

    int messageCount = 0;
    // printf("I am a child process with my pid = %d and process name = %s\n", getpid(), processName);
    while (1)
    {
        // printf("I am a child process with my pid = %d and process name = %s and my semIndex is %d\n", getpid(), processName, semIndex);
        //Wait blocked until signaled by P process
        struct sembuf sem_op = {semIndex, -1, 0};
        if (semop(semid, &sem_op, 1) == -1)
        {
            perror("semop (wait)");
            exit(1);
        }

        SharedMemory *shm = (SharedMemory *)shmat(shmid, NULL, 0);
        if (shm == (SharedMemory *)-1)
        {
            perror("shmat");
            exit(1);
        }

        if (strcmp(shm->message, "kill") == 0)
        {
            printf("I am Process %s with pid = %d and semIndex %d and acksemIndex %d and I m gonna exit now. I received termination message: %s. I received %d messages and last %d cycles\n", processName, getpid(), semIndex, ackSemIndex, shm->message, messageCount, shm->FinalCycle - StartCycle);

            shmdt(shm); //detach
            
            //Signal the P process that read the message 
            struct sembuf ack_op = {ackSemIndex, 1, 0};
            if (semop(semid, &ack_op, 1) == -1)
            {
                perror("semop (acknowledge)");
                exit(1);
            }
            //Exit
            exit(0);
        }
        else
        {
            printf("Process %s with pid = %d received line: %s\n", processName, getpid(), shm->message);
            messageCount++; //increase the messages counter

            shmdt(shm); //detach

            struct sembuf ack_op = {ackSemIndex, 1, 0};
            if (semop(semid, &ack_op, 1) == -1)
            {
                perror("semop (acknowledge)");
                exit(1);
            }
        }
    }
}

// Print the active processes in each cycle of P process for monitoring reasons .
void printProcesses(Process Processes[], int ActiveProcessCount)
{
    if (ActiveProcessCount == 0)
    {
        printf("No active processes.\n");
        return;
    }

    printf("Active Processes:\n");
    printf("PID\tProcess Name\tSemaphore Index\tAcknowledgment Index\tStarted Cycle\n");
    for (int i = 0; i < ActiveProcessCount; i++)
    {
        if (Processes[i].pid != 0) // Some processes maybe not active anymore. So we are need to implement thiis "if statement" validation.
            printf("%d\t\t%s\t\t%d\t\t%d\t\t%d\n",
                   Processes[i].pid,
                   Processes[i].processName,
                   Processes[i].semIndex,
                   Processes[i].acksemIndex,
                   Processes[i].StartedCycle);
    }
}
// Only for remaining processes at the end of the programm
void terminateAllProcesses(Process Processes[], int ActiveProcessCount)
{
    for (int i = 0; i < ActiveProcessCount; i++)
    {
        if (Processes[i].pid != 0){
        kill(Processes[i].pid, SIGTERM);
        printf("Terminated process %s with pid = %d\n", Processes[i].processName, Processes[i].pid);
        }
    }
}