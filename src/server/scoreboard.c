#include "scoreboard.h"
#include "string.h"

bool scoreboard_init(Scoreboard *scoreboard, int nTopics)
{
    scoreboard->nElements = 0;
    scoreboard->nTopics = nTopics;

    scoreboard->score_list = malloc(sizeof(DNode *) * nTopics);
    if (!scoreboard->score_list)
        return false;

    memset(scoreboard->score_list, 0, sizeof(DNode *) * nTopics);

    return false;
}

void scoreboard_destroy(Scoreboard *scoreboard)
{
    for (int i = 0; i < scoreboard->nTopics; i++)
        if (scoreboard->score_list[i])
            listDoubly_destroy(scoreboard->score_list[i], scoreboard_scoreDestroy);

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
    
    Score * tmp = malloc(sizeof(Score));
    if (!tmp)
        return NULL;

    tmp->name = NULL;
    tmp->score = 0;

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
    return listDoubly_append(&current, tmp);
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
        return -1;
    else if (a->score == b->score)
        return 0;
    else
        return 1;
}

void scoreboard_print(Scoreboard *scoreboard)
{
    for (int i = 0; i < scoreboard->nTopics; i++)
    {
        printf("scoreboard %d:\n", i);
        for (DNode * tmp = scoreboard->score_list[i]; tmp; tmp = tmp->next)
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
