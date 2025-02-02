#define _DEFAULT_SOURCE
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h> 
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "topic.h"
#include "util.h"

// #define DEBUG_PATH
// #define DEBUG_TOPIC
// #define DEBUG_QUESTION

bool directoryCreate(char * buffer_path, char * directory)
{
    int endline = strlen(buffer_path);
    strncat(buffer_path, directory, NAME_MAX);

    if (mkdir(buffer_path, 0755) < 0)
        if (errno != EEXIST)
        {
            perror("Errore nella creazione delle directory");
            return false;
        }

    buffer_path[endline] = '\0';
    return true;
}

bool topicsInit(TopicsContext *context)
{
    context->nTopics = 0;
    context->topics = NULL;

    if (!(executablePath(context->directory)))
        return true;

    if (!parentDirectory(context->directory))
        return true;

    strcat(context->directory, DATA_DIR);

    #ifdef DEBUG_PATH
        printf("directory: %s\n", context->directory); // DEBUG
    #endif
    
    if (!directoryCreate(context->directory, "") ||
        !directoryCreate(context->directory, TOPICS_DIR) ||
        !directoryCreate(context->directory, USERS_DIR))
        return false;

    return false;
}

bool topicsLoader(TopicsContext *context)
{
    #ifdef DEBUG_PATH
        printf("directory: %s\n", context->directory); // DEBUG
    #endif

    int endline = strlen(context->directory);
    strncat(context->directory, TOPICS_DIR, NAME_MAX);
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

    context->topicsString = messageArray(context->nTopics);

    for (int i = 0; i < context->nTopics; i++)
    {
    #ifdef DEBUG_TOPIC
            printf("topic %d: %s\n", i, context->topics[i].name);
    #endif
        topicLoad(context->directory, &context->topics[i]);
        topic_name(context->topics[i].name);
        messageString(&context->topicsString->messages[i], context->topics[i].name, false);
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

    char * line = NULL,
         * current_question = NULL;
    size_t alloc_len;
    while( getline(&line, &alloc_len, file) != -1)
    {
        // Ignora linee vuote, valutazione in corto circuito
        if ((line[0] == '\n' || line[1] == '\n'))
        {
            if (current_question)
                free(current_question);
            current_question = NULL;
            continue;
        }

        newlineReplace(line);

        #ifdef DEBUG_QUESTION
            printf("line %d/%d: _%s_\n", (int) strlen(line), (int) alloc_len, line); // DEBUG
        #endif

        if (!current_question)
        {
            current_question = line;
        }
        else
        {
            Question * new_question = malloc( sizeof(Question) );
            new_question->question = current_question;
            new_question->answer = line;
            list_append(&topic->questions, new_question);
            
            current_question = NULL;
            topic->nQuestions++;
        }

        line = NULL;
    }

    if (current_question)
        free(current_question);

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
    
    messageArrayDestroy(&context->topicsString);

    free(context->topics);
    
    context->nTopics = 0;
    return;
}

bool *topicsUnplayed(TopicsContext *context, char *user)
{
    int endline = strlen(context->directory);
    
    char *path = context->directory;
    strncat(path, USERS_DIR, NAME_MAX);
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
    strncat(path, USERS_DIR, NAME_MAX);
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
