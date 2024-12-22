#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h> 
#include <unistd.h>
#include "topic.h"
#include "util.h"

// #define DEBUG_PATH
// #define DEBUG_TOPIC
// #define DEBUG_QUESTION

// Nel mio caso dirent.h non include alcune definizioni
#ifndef DT_REG
#define DT_REG 8    /* File regolare */
#endif

bool topicsInit(TopicsContext *context, char * directory)
{
    context->nTopics = 0;
    context->topics = NULL;

    if (!(executablePath(context->directory)))
        return false;

    if (!parentDirectory(context->directory))
        return false;

    strcat(context->directory, directory);

#ifdef DEBUG_PATH
    printf("directory: %s\n", context->directory); // DEBUG
#endif

    return true;
}

bool topicsLoader(TopicsContext *context)
{
#ifdef DEBUG_PATH
    printf("directory: %s\n", context->directory); // DEBUG
#endif

    int endline = strlen(context->directory);
    strncat(context->directory, "./topics/", NAME_MAX);

    DIR * stream = opendir(context->directory);
    struct dirent *file;

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
            strncpy(current->name, file->d_name, NAME_MAX);
            current->questions = NULL;
        }
        closedir(stream);
    }

    qsort(context->topics, context->nTopics, sizeof(Topic), topics_compare);

    for (int i = 0; i < context->nTopics; i++)
    {
#ifdef DEBUG_TOPIC
         printf("topic %d: %s\n", i, context->topics[i].name);
#endif
        topicLoad(context->directory, &context->topics[i]);
        removeExtension(context->topics[i].name);
    }

    context->directory[endline] = '\0';

#ifdef DEBUG_PATH
    printf("directory: %s\n", context->directory); // DEBUG
#endif

    return false;
}

bool topicLoad(char * path, Topic * topic)
{
#ifdef DEBUG_PATH
    printf("directory: %s\n", path);
#endif

    FILE *file;

    int endline = strlen(path);
    strncat(path, topic->name, NAME_MAX);

#ifdef DEBUG_PATH
    printf("directory: %s\n", path);
#endif

    if (!(file = fopen(path, "r")))
        return false;

    Question * new_question = NULL;
    size_t alloc_len;
    for (char * line = NULL; getline(&line, &alloc_len, file) != -1; line = NULL)
    {
        // Ignora linee vuote, valutazione in corto circuito
        if ((line[0] == '\n' || line[1] == '\n'))
        {
            new_question = NULL;
            continue;
        }

        int len = strlen(line);
        if (len >= 2 && line[len - 2] == '\r') // Caso codifica Windows
            line[len - 2] = '\0';
        else if (line[len - 1] == '\n') // Rimuovo il carattere di nuova linea
            line[len - 1] = '\0';

#ifdef DEBUG_QUESTION
        printf("line %d/%d: _%s_\n", len, (int) alloc_len, line); // DEBUG
#endif

        if (!new_question)
        {
            new_question = malloc( sizeof(Question) );
            list_append(&topic->questions, new_question);
            new_question->question = line;
            new_question->answer = NULL; // Nel caso la risposta non venisse caricata
        }
        else
        {
            new_question->answer = line;
            new_question = NULL; // Verrà create una nuova domanda per prossima linea
        }
    }
    fclose(file);

    path[endline] = '\0';

    return false;
}

void topics_questionDestroy(void * data)
{
    if (!data)
        return;
    Question * question = (Question *) data;
    if (question->question)
        free(question->question);
    if (question->answer)
        free(question->answer);
}

// Ritorno il puntatore al primo carattere alfabetico
char *topic_name(char *name)
{
    for (int i = 0; name[i] != '\0'; i++)
        if ( (name[i] >= 'A' && name[i] <= 'Z') ||
         (name[i] >= 'a' && name[i] <= 'z'))
            return name + i;
    return NULL;
}

void topicsFree(TopicsContext *context)
{
    if (!context->nTopics)
        return;

    for (int i = 0; i < context->nTopics; i++)
        list_destroy(context->topics[i].questions, topics_questionDestroy);
        
    free(context->topics);
    
    context->nTopics = 0;
    return;
}

bool *topicsUnplayed(TopicsContext *context, char *user)
{
    return NULL;
}

bool topicPlayed(TopicsContext *context, char *topic, char *user)
{
    return false;
}

void topic_list_print_question(void * data)
{
    Question * tmp = (Question *) data;
    printf("_%s_: _%s_\n", tmp->question, tmp->answer);
}

int topics_compare(const void *a, const void *b){
    Topic * topic_a = (Topic *) a,
     * topic_b = (Topic *) b;
    return strcmp(topic_a->name, topic_b->name);
}
