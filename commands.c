#include "commands.h"
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

// Parsing and save the config text file
Command *parseConfig(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening file");
        return NULL;
    }

    Command *head = NULL;
    Command *tail = NULL;

    int timestamp;
    char processName[100];
    char command[100];

    while (1)
    {
        int itemsRead = fscanf(file, "%d %99s %99s", &timestamp, processName, command);

        if (itemsRead == EOF)
            break;

        Command *newCommand = malloc(sizeof(Command));
        if (!newCommand)
        {
            perror("Error allocating memory for Command");
            fclose(file);
            return head; // Return the list created so far.
        }

        newCommand->timestamp = timestamp;

        // Allocate and copy processName
        newCommand->processName = malloc(strlen(processName) + 1);
        if (!newCommand->processName)
        {
            perror("Error allocating memory for processName");
            free(newCommand);
            fclose(file);
            return head;
        }
        strcpy(newCommand->processName, processName);

        // Last line !!! (EXIT)
        if (itemsRead == 2)
        {
            // If only two words, treat as "EXIT" logic
            newCommand->command = malloc(5); // Length of "EXIT" + 1 for '\0'
            if (!newCommand->command)
            {
                perror("Error allocating memory for command");
                free(newCommand->processName);
                free(newCommand);
                fclose(file);
                return head;
            }
            strcpy(newCommand->command, "EXIT");
        }
        else
        {
            newCommand->command = malloc(strlen(command) + 1);
            if (!newCommand->command)
            {
                perror("Error allocating memory for command");
                free(newCommand->processName);
                free(newCommand);
                fclose(file);
                return head;
            }
            strcpy(newCommand->command, command);
        }

        newCommand->next = NULL;

        // Add to the list
        if (tail)
        {
            tail->next = newCommand;
            tail = newCommand;
        }
        else
        {
            head = tail = newCommand;
        }
    }

    fclose(file);
    return head;
}

// Print the commands. for debugging reasons
void printCommands(Command *head)
{
    int counter = 0;
    Command *current = head;
    while (current != NULL)
    {
        counter++;
        printf("Timestamp: %d, Process: %s, Command: %s\n", current->timestamp, current->processName, current->command);
        current = current->next;
    }
}

// Handling a line/command of config.txt file.
int processCommand(Command *command, int maxProcesses, Process Processes[], int *ActiveProcessCount, int cycleCounter, int semid, int shmid)
{
    // SPAWN
    if (strcmp(command->command, "S") == 0)
    {
        if (*ActiveProcessCount >= maxProcesses)
        {
            printf("Cannot spawn more processes. Maximum limit reached.\n");
            return 1;
        }
        // Find the first "empty" position
        int Sem_index = 0;
        for (int i = 0; i <= *ActiveProcessCount; i++)
        {
            if (Processes[i].pid == 0)
            {
                Sem_index = i; //This will be the first semaphore index that child will be attached
                break;
            }
        }

        pid_t pid = fork();
        if (pid == 0)
        {

            childProcess(semid, shmid, Sem_index, Sem_index + maxProcesses, command->processName, cycleCounter);
            exit(0);
        }
        else if (pid > 0)
        {
            //initialize the metadata of the process
            Processes[Sem_index].pid = pid;
            strncpy(Processes[Sem_index].processName, command->processName, PROCESSNAME_SIZE);
            Processes[Sem_index].semIndex = Sem_index;
            Processes[Sem_index].acksemIndex = Sem_index + maxProcesses;
            Processes[Sem_index].StartedCycle = cycleCounter;

            (*ActiveProcessCount)++;
            printf("Spawned process %s with pid = %d\n\n\n\n", command->processName, pid);
        }
        else
        {
            perror("Fork failed");
        }
    }
    else if (strcmp(command->command, "T") == 0) //TERMINATE
    {
        int found = 0;
        for (int i = 0; i < maxProcesses; i++)
        {
            if ((strcmp(Processes[i].processName, command->processName) == 0) && Processes[i].pid != 0)
            {
                found = 1;

                SharedMemory *shm = (SharedMemory *)shmat(shmid, NULL, 0);
                if (shm == (SharedMemory *)-1)
                {
                    perror("shmat");
                    exit(1);
                }
                snprintf(shm->message, SHM_SIZE, "kill");
                shm->FinalCycle = cycleCounter;
                shmdt(shm);

                struct sembuf sem_op = {Processes[i].semIndex, 1, 0};
                if (semop(semid, &sem_op, 1) == -1)
                {
                    perror("semop (signal)");
                    exit(1);
                }

                struct sembuf ack_op = {Processes[i].acksemIndex, -1, 0};
                if (semop(semid, &ack_op, 1) == -1)
                {
                    perror("semop (acknowledge)");
                    exit(1);
                }

                int status;
                pid_t terminated_pid = Processes[i].pid;
                int ret = waitpid(terminated_pid, &status, 0);
                if (ret == -1)
                {
                    perror("waitpid");
                }
                else if (ret == 0)
                {
                    printf("Process %d has not exited yet.\n", terminated_pid);
                }
                else
                {
                    printf("Process %d exited with status %d.\n", terminated_pid, WEXITSTATUS(status));
                }
                // The process terminated so we make this pid equal to zero. 
                // So this position is considered empty now. 
                Processes[i].pid = 0;
                (*ActiveProcessCount)--;
                break;
            }
        }
        if (!found)
        {
            printf("Process %s does not exist.\n\n\n\n", command->processName);
        }
    }
    else if (strcmp(command->command, "EXIT") == 0) //EXIT
    {
        printf("End of config file\n\n");
        return 0;
    }
    return 1;
}

// Function for random selection of an active process and a text line from the txt inputed file
void sendTextToChild(int semid, int shmid, TextLine *textLines, int lines, Process Processes[], int ActiveProcessCount, int max_processes)
{
    //validation 
    if (ActiveProcessCount == 0 || textLines == NULL)
    {
        return;
    }
    // select randomly a line from the text file 
    // int lineCount = 0;
    TextLine *current = textLines;
    int randomLineIndex = rand() % lines;
    
    for (int i = 0; i < randomLineIndex; i++)
    {
        current = current->next;
    }
    //select a random process
    int randomIndex = rand() % max_processes;
    while (Processes[randomIndex].pid == 0)
    {
        randomIndex = rand() % max_processes;
    }

    Process *selectedProcess = &Processes[randomIndex];
    //Shared memory segment
    SharedMemory *shm = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (SharedMemory *)-1)
    {
        perror("shmat");
        exit(1);
    }
    strncpy(shm->message, current->line, SHM_SIZE);
    shmdt(shm);

    //Signal to child to unblock
    struct sembuf sem_op = {selectedProcess->semIndex, 1, 0};
    if (semop(semid, &sem_op, 1) == -1)
    {
        perror("semop (signal)");
        exit(1);
    }
    //Wait for child signal 
    struct sembuf ack_op = {selectedProcess->acksemIndex, -1, 0};
    if (semop(semid, &ack_op, 1) == -1)
    {
        perror("semop (wait for acknowledgment)");
        exit(1);
    }
}