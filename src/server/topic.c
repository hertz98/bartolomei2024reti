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
#include <ctype.h>
#include "topic.h"
#include "util.h"

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
        return false;

    if (!parentDirectory(context->directory))
        return false;

    strcat(context->directory, DATA_DIR);
    
    if (!directoryCreate(context->directory, "") ||
        !directoryCreate(context->directory, TOPICS_DIR) ||
        !directoryCreate(context->directory, USERS_DIR))
        return false;

    return true;
}

bool topicsLoader(TopicsContext *context)
{
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
            if (!context->topics)
                return false;

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

    context->topicsString = messageArray(context->nTopics + 1);
    if (!context->topicsString)
        return false;

    for (int i = 0; i < context->nTopics; i++)
    {
        topicLoad(context->directory, &context->topics[i]);
        topic_name(context->topics[i].name);
        messageString(&context->topicsString->messages[i], context->topics[i].name, false);
    }

    context->directory[endline] = '\0';

    return true;
}

bool topicLoad(char * path, Topic * topic)
{
    FILE *file;

    int endline = strlen(path);
    strncat(path, topic->name, NAME_MAX);

    if (!(file = fopen(path, "r")))
        return false;

    char * current_question = NULL;
    char *line = NULL;
    size_t alloc_len = 0;
    while(getline(&line, &alloc_len, file) != -1)
    {
        // Ignora linee vuote, valutazione in corto circuito
        if ((line[0] == '\n' || // Linee vuote
                (line[0] == '\r' && line[1] == '\n'))) // caso Windows, valutazione in corto circuito
        {
            if (current_question)
                free(current_question);
            current_question = NULL;
            continue;
        }

        newlineReplace(line);

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

    bool *unplayed = (bool *) malloc(context->nTopics);
    for (int i = 0; i < context->nTopics; i++)
        unplayed[i] = true;

    FILE *file;
    if ((file = fopen(path, "r")))
    {
        char *line = NULL;
        size_t alloc_len = 0;
        while(getline(&line, &alloc_len, file) != -1)
        {
            if ((line[0] == '\n' || // Linee vuote
                    (line[0] == '\r' && line[1] == '\n'))) // caso Windows, valutazione in corto circuito
                continue;

            newlineReplace(line);

            if (isdigit( (uint8_t) line[0])) // In questo caso ignoro i punteggi
                continue;

            for (int i = 0; i < context->nTopics; i++)
                if (!strcmp(line, context->topics[i].name ))
                {
                    unplayed[i] = false;
                    break;
                }

        }
        free(line);
        fclose(file);
    }
    
    context->directory[endline] = '\0';

    return unplayed;
}

/* topicsPlayed
 * Per ciascuna riga non vuota:
 *   se non è un numero la considero un topic e se presente la segno come giocato
 *   se è un numero la considero il punteggio dell'ultimo topic giocato (nella riga precedente)
 * 
 * - i topic giocati senza aver trovato il punteggio avranno punteggio 0
 */
int *topicsPlayed(TopicsContext *context, char *user)
{
    int endline = strlen(context->directory);
    
    char *path = context->directory;
    strncat(path, USERS_DIR, NAME_MAX);
    strncat(path, user, NAME_MAX);
    strncat(path, ".txt", NAME_MAX);

    int *played = (int *) malloc(sizeof(int) * context->nTopics);
    for (int i = 0; i < context->nTopics; i++)
        played[i] = -1;

    FILE *file;
    if ((file = fopen(path, "r")))
    {
        int last = -1; // Ricorda l'ultimo indice
        char *line = NULL;
        size_t alloc_len = 0;
        while(getline(&line, &alloc_len, file) != -1)
        {
            if ((line[0] == '\n' || // Linee vuote
                (line[0] == '\r' && line[1] == '\n'))) // caso Windows, valutazione in corto circuito
            {
                last = -1;
                continue;
            }

            newlineReplace(line);

            if (last != -1 && played[last] == 0 && isdigit( (uint8_t) line[0])) // Punteggi
            {
                if (sscanf(line, "%d", &played[last]) > 0)
                {
                    if (played[last] < 0) // non esistono punteggi minori di 0
                        played[last] = 0;
                    last = -1;
                }
                continue;
            }

            for (int i = 0; i < context->nTopics; i++)
                if (!strcmp(line, context->topics[i].name ))
                {
                    last = i;
                    played[i] = false;
                    break; // break internal loop
                }

        }
        free(line);
        fclose(file);
    }
    
    context->directory[endline] = '\0'; // ripristino il percorso originale

    return played;
}

bool topicMakePlayed(TopicsContext *context, char *user, int i_topic, int score)
{
    if (i_topic >= context->nTopics)
        return false;

    int endline = strlen(context->directory);
    
    char *path = context->directory;
    strncat(path, USERS_DIR, NAME_MAX);
    strncat(path, user, NAME_MAX);
    strncat(path, ".txt", NAME_MAX);

    FILE *file;
    if (!(file = fopen(path, "a")))
        return false;

    if (score == -1)
        fprintf(file, "%s\n", context->topics[i_topic].name);
    else
        fprintf(file, "%d\n\n", score);

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
