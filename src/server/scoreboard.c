#define _DEFAULT_SOURCE
#include "scoreboard.h"
#include "string.h"

bool scoreboard_init(Scoreboard *scoreboard, int nTopics)
{
    scoreboard->nElements = 0;
    scoreboard->nTopics = nTopics;

    scoreboard->current = malloc(sizeof(DNode *) * nTopics);
    if (!scoreboard->current)
        return false;

    scoreboard->completed = malloc(sizeof(DNode *) * nTopics);
    if (!scoreboard->current)
        return false;

    memset(scoreboard->current, 0, sizeof(DNode *) * nTopics);
    memset(scoreboard->completed, 0, sizeof(DNode *) * nTopics);

    return true;
}

void scoreboard_destroy(Scoreboard *scoreboard)
{
    for (int i = 0; i < scoreboard->nTopics; i++)
        if (scoreboard->current[i])
            listDoubly_destroy(scoreboard->current[i], scoreboard_scoreDestroy);

    scoreboard->nElements = 0;
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
    for (int i = 0; i < scoreboard->nTopics; i++)
    {
        printf("scoreboard %d:\n", i);
        for (DNode * tmp = scoreboard->current[i]; tmp; tmp = tmp->next)
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