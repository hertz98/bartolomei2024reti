#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h> 
#include <stdio.h>
#include <unistd.h>
#include "topic.h"


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
    context->topics = NULL;
    if (readlink("/proc/self/exe", context->directory, PATH_BUFFER_SIZE) == -1)
        return false;
    if (!parentDirectory(context->directory))
        return false;
    strcat(context->directory, directory);
    return true;
}

bool topicsLoader(struct TopicsContext *context)
{
    DIR * stream = opendir(context->directory);
    struct dirent *file;

    printf("directory: %s\n", context->directory);
    if (stream)
    {
        while ((file = readdir(stream)) != NULL)
        {
            if (file->d_type != DT_REG)
                continue;

            context->nTopics++;
            context->topics = realloc(context->topics, sizeof(Topic) * context->nTopics);

            Topic * current = &context->topics[context->nTopics - 1];
            memset(current, 0, sizeof(Topic));
            strncpy(current->name, file->d_name, FILE_NAME_SIZE);
            current->questions = NULL;
        }
        closedir(stream);
    }

    for (int i = 0; i < context->nTopics; i++)
        printf("%s\n",context->topics[i].name);

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
