#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h> 
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

    while (n > 0 && path[n] != '/')
        n--;

    if (path[n] != '/')
        return false;

    path[ n + 1 ] = '\0';
    return true;
}

bool removeExtension(char * path)
{
    int n = strlen(path) - 1;

    while(n > 0 && path[ n ] != '.')
        n--;

    if (n == 0 || path[n - 1] == '/')
        return false;

    path[n] = '\0';

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

    for (int i = 0; i < context->nTopics; i++)
    {
        topicLoad(context, &context->topics[i]);
        removeExtension(context->topics[i].name);
    }

    return false;
}

void list_print_question(void * data)
{
    Question * tmp = (Question *) data;
    printf("%s: %s\n", tmp->question, tmp->answer);
}

bool topicLoad(struct TopicsContext *context, struct Topic * topic)
{
    FILE *file;

    char file_path[4096];
    strcpy(file_path, context->directory);
    strncat(file_path, topic->name, FILE_NAME_SIZE);

    if (!(file = fopen(file_path, "r")))
        return false;

    Question * new_question = NULL;
    size_t n;
    for (char * line = NULL; getline(&line, &n, file) != -1; line = NULL)
    {
        if ((line[0] == '\0')) // Ignore empty lines
        {
            new_question = NULL;
            continue;
        }

        line[strlen(line) - 1] = '\0'; // Rimuovo il carattere di nuova line

        if (!new_question)
        {
            new_question = malloc( sizeof(Question) );
            list_append(&topic->questions, new_question);
            new_question->question = line;
            new_question->answer = ""; // Nel caso la risposta non venisse caricata
        }
        else
        {
            new_question->answer = line;
            new_question = NULL;
        }
    }
    fclose(file);

    list_print(topic->questions, list_print_question);

    return false;
}

void topics_questionDestroy(void * p)
{
    Question * question = (Question *) p;
    if (question->question)
        free(question->question);
    if (question->answer)
        free(question->answer);
}

void topicsFree(struct TopicsContext *context)
{
    if (!context->nTopics)
        return;

    for (int i = 0; i < context->nTopics; i++)
        list_destroy(context->topics[i].questions, topics_questionDestroy);
    free(context->topics);

    context->nTopics = 0;
    return;
}
