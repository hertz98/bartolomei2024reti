#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h> 
#include <unistd.h>
#include "topic.h"
#include "util.h"

#ifndef DT_REG
#define DT_REG 8
#endif

int topics_compare(const void *a, const void *b){
    Topic * topic_a = (Topic *) a,
     * topic_b = (Topic *) b;
    return strcmp(topic_a->name, topic_b->name);
}

bool topicsInit(struct TopicsContext *context, char * directory)
{
    context->nTopics = 0;
    context->topics = NULL;
    context->directory = NULL;

    if (!(context->directory = executablePath()))
        return false;

    if (!parentDirectory(context->directory))
        return false;

    strcat(context->directory, directory);

    return true;
}

bool topicsLoader(struct TopicsContext *context)
{
    //printf("directory: %s\n", context->directory); // DEBUG
    
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
            strncpy(current->name, file->d_name, FILE_NAME_SIZE);
            current->questions = NULL;
        }
        closedir(stream);
    }

    qsort(context->topics, context->nTopics, sizeof(Topic), topics_compare);

    // for (int i = 0; i < context->nTopics; i++) // DEBUG
    //     printf("%s\n",context->topics[i].name);

    for (int i = 0; i < context->nTopics; i++)
    {
        topicLoad(context, &context->topics[i]);
        removeExtension(context->topics[i].name);
    }

    return false;
}

void topic_list_print_question(void * data)
{
    Question * tmp = (Question *) data;
    printf("_%s_: _%s_\n", tmp->question, tmp->answer);
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

        // printf("line %d/%d: _%s_\n", len, (int) alloc_len, line); // DEBUG

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

    return false;
}

void topics_questionDestroy(void * p)
{
    if (!p)
        return;
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

    if(context->directory)
        free(context->directory);

    context->nTopics = 0;
    return;
}
