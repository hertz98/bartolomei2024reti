#define _DEFAULT_SOURCE
#include "string.h"
#include "util.h"
#include "scoreboard.h"
#include "string.h"

bool scoreboard_init(Scoreboard *scoreboard, TopicsContext * topics)
{
    scoreboard->nTopics = topics->nTopics;

    for (int i = 0; i < SCOREBOARD_SIZE; i++)
    {
        scoreboard->scores[i] = malloc(sizeof(DNode *) * topics->nTopics);
        if (!scoreboard->scores[i])
            return false;
        memset(scoreboard->scores[i], 0, sizeof(DNode *) * scoreboard->nTopics);
    
        scoreboard_serialize_init(&scoreboard->serialized[i], topics->nTopics);

    }
    scoreboard_serialize_update(scoreboard, topics);

    return true;
}

bool scoreboard_serialize_init(SerializedScoreboard * scoreboard, int size)
{
    scoreboard->modified = malloc(sizeof(bool) * size);
    if (!scoreboard->modified)
        return false;
    
    scoreboard->string = malloc(sizeof(char *) * size);
    if (!scoreboard->string)
        return false;
    
    for (int i = 0; i < size; i++)
    {
        scoreboard->string[i] = malloc(DEFAULT_SCOREBOARD_ALLOCATION);
        if (!scoreboard->string[i])
            return false;
        scoreboard->string[i][0] = '\0';
    }

    scoreboard->serialized_lenght = malloc(sizeof(int) * size);
    if (!scoreboard->serialized_lenght)
        return false;
    
    scoreboard->serialized_allocated = malloc(sizeof(int) * size);
    if (!scoreboard->serialized_allocated)
        return false;
    
    // Devo impostare la classifica come modificata se voglio che venga aggiornata subito
    memset(scoreboard->modified, 1, sizeof(bool) * size);
    memset(scoreboard->serialized_lenght, 0, sizeof(int) * size);
    
    for (int i = 0; i < size; i++)
        scoreboard->serialized_allocated[i] = DEFAULT_SCOREBOARD_ALLOCATION;

    return true;
}

void scoreboard_destroy(Scoreboard *scoreboard)
{
    for (int i = 0; i < SCOREBOARD_SIZE; i++)
    {
        if (scoreboard->scores[i])
            for (int j = 0; j < scoreboard->nTopics; j++)
                if (scoreboard->scores[i][j])
                {
                    listDoubly_destroy(scoreboard->scores[i][j], scoreboard_scoreDestroy);
                    scoreboard->scores[i][j] = NULL;
                }
        
        if (scoreboard->serialized->modified)
            free(scoreboard->serialized->modified);
        scoreboard->serialized->modified = NULL;
        
        for (int j = 0; j < scoreboard->nTopics; j++)
            if (scoreboard->serialized[i].string[j])
                {
                    free(scoreboard->serialized[i].string[j]);
                    scoreboard->serialized[i].string[j] = NULL;
                }
        
        if (scoreboard->serialized->serialized_allocated)
            free(scoreboard->serialized->serialized_allocated);
        scoreboard->serialized->serialized_allocated = NULL;

        if (scoreboard->serialized->serialized_lenght)
            free(scoreboard->serialized->serialized_lenght);
        scoreboard->serialized->serialized_lenght = NULL;
    }
}

void scoreboard_scoreDestroy(void *p)
{
    Score * score = p;

    if (score->name)
        free(score->name);

    free(score);
}

DNode *scoreboard_get(DNode **score_list, char * name)
{
    if (!score_list || !name)
        return NULL;
    
    Score * tmp = scoreboard_newScore(name, 0);

    if (!*score_list)
        return listDoubly_append(score_list, tmp);

    DNode *current = *score_list;
    for (; current && current->next; current = current->next)
    {
        char * score_name = ((Score *) current->data)->name;
        if ( score_name && !strcmp( score_name , name) )
            return current;
    }
    
    // Evito di ripercorrere tutta la intera lista
    // Aggiungo in fondo perchè è probabile che il punteggio sia 0 inizialmente
    DNode *ret;
    if (!(ret = listDoubly_append(&current, tmp)))
        free(tmp);
    return ret;
}

Score * scoreboard_newScore(char * name, int score)
{
    Score * tmp = malloc(sizeof(Score));

    if (!tmp)
        return NULL;

    tmp->name = name;
    tmp->score = score;

    return tmp;
}

void scoreboard_serialize_update(Scoreboard *scoreboard, TopicsContext *topics)
{
    for (int i = 0; i < SCOREBOARD_SIZE; i++)
    {
        SerializedScoreboard *current = &scoreboard->serialized[i];

        for (int t = 0; t < scoreboard->nTopics; t++)
            if (current->modified[t])
            {
                char buffer[512];

                #ifdef PRINT_TOPIC_NAMES_ALWAYS
                    if (i == SCR_CURRENT)
                        snprintf(buffer, sizeof(buffer), "Punteggio tema \"%d - %s\"\n", t + 1, topics->topics[t].name);
                    else
                        snprintf(buffer, sizeof(buffer), "Quiz tema \"%d - %s\" completato\n", t + 1, topics->topics[t].name);
                #else
                    if (i == SCR_CURRENT)
                        snprintf(buffer, sizeof(buffer), "Punteggio tema %d\n", i + 1);
                    else
                        snprintf(buffer, sizeof(buffer), "Quiz tema %d completato\n", i + 1);
                #endif
                
                current->serialized_lenght[t] = strcpyResize(&current->string[t], 
                                                            buffer, 
                                                            &current->serialized_allocated[t], 
                                                            0);

                for (DNode * node = scoreboard->scores[i][t]; node; node = node->next)
                {
                    Score *score = node->data;

                    #ifdef PRINT_COLON_SCORE
                        snprintf(buffer, sizeof(buffer), "- %s: %d\n", current->name, current->score);
                    #else
                        snprintf(buffer, sizeof(buffer), "- %s %d\n", score->name, score->score);
                    #endif

                    current->serialized_lenght[t] += strcpyResize(&current->string[t], 
                                                                buffer, 
                                                                &current->serialized_allocated[t], 
                                                                current->serialized_lenght[t]);
                }

                // current->serialized_lenght[t]++; // The last NULL caracher
                current->modified[t] = false;
            }   
    }
}

void scoreboard_increase(Score *score)
{
    if (!score)
        return;

    score->score++;
}

int scoreboard_scoreCompare(void *a_ptr, void *b_ptr)
{
    Score *a = a_ptr;
    Score *b = b_ptr;
    if (a->score < b->score)
        return +1;
    else if (a->score == b->score)
        return 0;
    else
        return -1;
}

void scoreboard_print(Scoreboard *scoreboard)
{
    for (int i = 0; i < SCOREBOARD_SIZE; i++)
        for (int t = 0; t < scoreboard->nTopics; t++)
        {
            printf("scoreboard %d.%d:\n", i, t);
            for (DNode * tmp = scoreboard->scores[i][t]; tmp; tmp = tmp->next)
            {
                scoreboard_scorePrint(tmp->data);
                printf("\n");
            }
            printf("\n");
        }
}

void scoreboard_scorePrint(void *score)
{
    printf("- %s: %d", ((Score *) score)->name, ((Score *) score)->score);
}