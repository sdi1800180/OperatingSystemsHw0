#include "text.h"
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

#define BUFFER_SIZE 4096

// Read and save the txt file
TextLine *readTextFile(const char *filename)
{
    int counter = 0;
    int fd = open(filename, O_RDONLY);//read only
    if (fd == -1)
    {
        perror("Failed to open text file");
        return NULL;
    }

    TextLine *head = NULL;
    TextLine *tail = NULL;

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytesRead] = '\0';
        char *line = strtok(buffer, "\n");

        while (line != NULL)
        {
            TextLine *newLine = (TextLine *)malloc(sizeof(TextLine));
            newLine->line = strdup(line); 
            newLine->next = NULL;

            if (head == NULL) //if list is empty 
            {
                head = newLine;
                tail = newLine;
            }
            else              //if not empty 
            {
                tail->next = newLine;
                tail = newLine;
            }
            line = strtok(NULL, "\n"); //Get the next line of buffer
            counter ++;
        }
    }

    close(fd);
    head->linecounter = counter; //In order to pass it in SendTextToChild as an argument
    return head;
}
// For free the used memory
void freeTextLines(TextLine *head)
{
    
    TextLine *current = head;
    while (current != NULL)
    {
        TextLine *next = current->next;
        free(current->line);
        
        free(current);
        current = next;
    }
}