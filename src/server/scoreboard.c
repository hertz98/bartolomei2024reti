#include "scoreboard.h"
#include "string.h"

bool scoreboard_init(Scoreboard *scoreboard, int nTopics)
{
    scoreboard->nElements = 0;
    scoreboard->nTopics = nTopics;

    scoreboard->score_list = malloc(sizeof(Node *) * nTopics);
    if (!scoreboard->score_list)
        return false;

    memset(scoreboard->score_list, 0, sizeof(Node *) * nTopics);

    return false;
}

void scoreboard_destroy(Scoreboard *scoreboard)
{
    for (int i = 0; i < scoreboard->nTopics; i++)
        if (scoreboard->score_list[i])
            list_destroy(scoreboard->score_list[i], scoreboard_scoreDestroy);

    scoreboard->nElements = 0;
}

void scoreboard_scoreDestroy(void *p)
{
    Score * score = p;

    if (score->name)
        free(score->name);

    free(score);
}

Score *scoreboard_get(Node **score_list, char * name)
{
    Node ** current = score_list;
    while( (*current) && (*current)->next)
    {
        char * tmp_name = ((Score *) (*current)->data)->name;
        if ( tmp_name && !strcmp( tmp_name , name) )
            return (*current)->data;

        (*current) = (*current)->next;
    }

    Score * tmp = malloc(sizeof(Score));
    if (!tmp)
        return NULL;
    
    return list_append(current, tmp)->data;
}

void scoreboard_increase(Score *score)
{
    if (!score)
        return;
    
    score->score++;
}
