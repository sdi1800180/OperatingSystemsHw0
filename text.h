#ifndef TEXT_H
#define TEXT_H

// Struct for save a line of text.txt file
typedef struct TextLine
{
    char *line;
    int linecounter; //For the head node in order to count the whole number of liens and pass it in SendTextToChild
    struct TextLine *next;
} TextLine;

// Respective functions
TextLine *readTextFile(const char *filename);
void freeTextLines(TextLine *head);

#endif //TEXT_H