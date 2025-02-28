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

Score *scoreboard_get(DNode **score_list, char * name)
{
    DNode ** current = score_list;
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
    
    return listDoubly_append(current, tmp)->data;
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
        printf("scoreboard %d\n", i);
        for (DNode * tmp = scoreboard->score_list[i]; tmp; tmp = tmp->next)
        {
            scoreboard_scorePrint(tmp);
            printf("\n");
        }
        printf("\n");
    }
}

void scoreboard_scorePrint(void *score)
{
    printf("%s: %d", ((Score *) score)->name, ((Score *) score)->score);
}
