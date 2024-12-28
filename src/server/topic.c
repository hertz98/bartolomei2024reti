#define _DEFAULT_SOURCE
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

bool topicsInit(TopicsContext *context, char * directory)
{
    context->nTopics = 0;
    context->topics = NULL;

    if (!(executablePath(context->directory)))
        return true;

    if (!parentDirectory(context->directory))
        return true;

    strcat(context->directory, directory);

    #ifdef DEBUG_PATH
        printf("directory: %s\n", context->directory); // DEBUG
    #endif

    return false;
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
            current->nQuestions = 0;
        }
        closedir(stream);
    }
    else
        return false;

    qsort(context->topics, context->nTopics, sizeof(Topic), topics_compare);

    for (int i = 0; i < context->nTopics; i++)
    {
    #ifdef DEBUG_TOPIC
            printf("topic %d: %s\n", i, context->topics[i].name);
    #endif
        topicLoad(context->directory, &context->topics[i]);
        topic_name(context->topics[i].name);
    }

    context->directory[endline] = '\0';

    #ifdef DEBUG_PATH
        printf("directory: %s\n", context->directory); // DEBUG
    #endif

    return true;
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
    char * line = NULL;
    size_t alloc_len;
    while( getline(&line, &alloc_len, file) != -1)
    {
        // Ignora linee vuote, valutazione in corto circuito
        if ((line[0] == '\n' || line[1] == '\n'))
        {
            new_question = NULL;
            continue;
        }

        newlineReplace(line);

        #ifdef DEBUG_QUESTION
            printf("line %d/%d: _%s_\n", (int) strlen(line), (int) alloc_len, line); // DEBUG
        #endif

        if (!new_question)
        {
            new_question = malloc( sizeof(Question) );
            list_append(&topic->questions, new_question);
            new_question->question = line;
            new_question->answer = NULL; // Nel caso la risposta non venisse caricata
            topic->nQuestions++;
        }
        else
        {
            new_question->answer = line;
            new_question = NULL; // Verrà create una nuova domanda per prossima linea
        }

        line = NULL;
    }
    free(line);
    fclose(file);

    path[endline] = '\0';

    return true;
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
    free(question);
}

void topic_name(char *name)
{
    if (!name[0])
        return;

    removeExtension(name);

    removeNumbering(name);

    return;
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
    int endline = strlen(context->directory);
    
    char *path = context->directory;
    strncat(path, "./users/", NAME_MAX);
    strncat(path, user, NAME_MAX);
    strncat(path, ".txt", NAME_MAX);
    
    #ifdef DEBUG_PATH
        printf("%s\n", path);
    #endif

    bool *unplayed = (bool *) malloc(context->nTopics);
    for (int i = 0; i < context->nTopics; i++)
        unplayed[i] = true;

    FILE *file;
    if ((file = fopen(path, "r")))
    {
        char * line = (char *) malloc(NAME_MAX * sizeof(char)); // getline() vuole una stringa allocata con la malloc()
        size_t alloc_len;
        while(getline(&line, &alloc_len, file) != -1)
        {
            if ((line[0] == '\n' || line[1] == '\n')) // Linee vuote
                continue;

            newlineReplace(line);

            for (int i = 0; i < context->nTopics; i++)
                if (!strcmp(line, context->topics[i].name ))
                    unplayed[i] = false;

        }
        free(line);
        fclose(file);
    }
    
    context->directory[endline] = '\0';

    return unplayed;
}

bool topicPlayed(TopicsContext *context, char *user, int i_topic)
{
    if (i_topic >= context->nTopics)
        return false;

    int endline = strlen(context->directory);
    
    char *path = context->directory;
    strncat(path, "./users/", NAME_MAX);
    strncat(path, user, NAME_MAX);
    strncat(path, ".txt", NAME_MAX);

    #ifdef DEBUG_PATH
        printf("%s\n", path);
    #endif

    char * topic = context->topics[i_topic].name;

    FILE *file;
    if (!(file = fopen(path, "a")))
        return false;

    fprintf(file, "%s\n", topic);

    fclose(file);

    context->directory[endline] = '\0';
    return true;
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
