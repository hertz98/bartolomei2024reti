#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h> 
#include <stdio.h>
#include "topic.h"
#include <unistd.h>


#ifndef DT_REG
#define DT_REG 8
#endif

bool parentDirectory(char * path)
{
    int n = strlen(path) - 1;

    if (path[n] == '/')
        n--;

    while (n-- > 1 && path[n] != '/');

    if (path[n] != '/')
        return false;

    path[ n + 1 ] = '\0';
    return true;
}

bool topicsInit(struct TopicsContext *context, char * directory)
{
    context->nTopics = 0;
    if (readlink("/proc/self/exe", context->directory, PATH_BUFFER_SIZE) == -1)
        return false;
    if (!parentDirectory(context->directory))
        return false;
    strcat(context->directory, directory);
    return true;
}

bool topicsLoader(struct TopicsContext *context)
{
    int count = 0;
    DIR * stream = opendir(context->directory);
    struct dirent *directory;

    printf("directory: %s\n", context->directory);
    if (stream)
    {
        while ((directory = readdir(stream)) != NULL)
        {
            if (directory->d_type != DT_REG)
                continue;
            printf("%s\n", directory->d_name);
            count++;
        }
        closedir(stream);
    }

    printf("count= %d\n", count);

    return false;
}

bool topicLoad(FILE * file, struct Topic *)
{
    // Scorro il file e mi ricavo il numero di domande/risposte valide
    // creo un buffer di dimensione QUESTION_MAX_SIZE che utilizzerò come locazione temporanea per la lettura del file
    // Creo lo array e riapro il file
    //foreach(riga){
        // scrivo la domanda/riposta nel buffer
        // malloc e la aggiungo alla struttura
        // se struttura completa, la aggiungo all'array o alla lista
    return false;
}

void topicsFree(struct Question ** topics)
{

    return;
}
