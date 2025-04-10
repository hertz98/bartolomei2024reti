#define _DEFAULT_SOURCE
#include "string.h"
#include "util.h"
#include "scoreboard.h"
#include "string.h"

bool scoreboard_init(Scoreboard *scoreboard, TopicsContext * topics)
{
    scoreboard->nTopics = topics->nTopics;

    // Alloco l'area di memoria dedicata a contenere i puntatori delle serializzazioni
    scoreboard->serialized = malloc(sizeof(SerializedScoreboard) * SCOREBOARD_SIZE * scoreboard->nTopics);
    if (!scoreboard->serialized)
        return false;

    for (int i = 0; i < SCOREBOARD_SIZE; i++)
    {
        // Alloco i puntatori alle liste di punteggi per ciascuna classifica (in gioco e completata)
        scoreboard->scores[i] = malloc(sizeof(DNode *) * scoreboard->nTopics);
        if (!scoreboard->scores[i])
            return false;
        memset(scoreboard->scores[i], 0, sizeof(DNode *) * scoreboard->nTopics);
    
        // Per ciascun topic inizializzo le serializzazioni
        for (int t = 0; t < scoreboard->nTopics; t++)
            if (!scoreboard_serialize_init(&scoreboard->serialized[ scoreboard_serialize_index(scoreboard, i, t) ]))
                return false;
    }
    
    // Ora che tutto e inizializzato creo le serializzazioni
    scoreboard_serialize_update(scoreboard, topics);

    return true;
}

bool scoreboard_serialize_init(SerializedScoreboard * scoreboard)
{
    scoreboard->string = malloc(sizeof(char) * DEFAULT_SCOREBOARD_SERIALIZE_ALLOCATION);
    if (!scoreboard->string)
        return false;

    scoreboard->string[0] = '\0';

    scoreboard->modified = true;

    scoreboard->serialized_allocated = DEFAULT_SCOREBOARD_SERIALIZE_ALLOCATION;

    scoreboard->serialized_lenght = 0;

    return true;
}

void scoreboard_destroy(Scoreboard *scoreboard)
{
    for (int i = 0; i < SCOREBOARD_SIZE; i++)
    {
        if (scoreboard->scores[i])
            for (int t = 0; t < scoreboard->nTopics; t++)
                if (scoreboard->scores[i][t])
                {
                    listDoubly_destroy(scoreboard->scores[i][t], scoreboard_scoreDestroy);
                    scoreboard->scores[i][t] = NULL;
                }
        
        free(scoreboard->scores[i]);
        scoreboard->scores[i] = NULL;
        
        for (int t = 0; t < scoreboard->nTopics; t++)
        {
            int index = scoreboard_serialize_index(scoreboard, i, t);
            if (scoreboard->serialized[index].string)
                free(scoreboard->serialized[index].string);
            scoreboard->serialized[index].string = NULL;
        }
    }

    if (scoreboard->serialized)
        free(scoreboard->serialized);
}

void scoreboard_scoreDestroy(void *p)
{
    Score * score = p;

    // Lascio al sistema operativo pulire i nomi, dovrei 
    // tenere traccia di quante volte è usata quella stringa
    // indipendentemente dai client, e non ne vale 

    // if (score->name)
    //     free(score->name);

    free(score);
}

DNode *scoreboard_get(Scoreboard * scoreboard, int index, int topic, char * name)
{
    if (topic < 0 || topic >= scoreboard->nTopics)
        return NULL;

    DNode ** score_list = &scoreboard->scores[index][topic];

    if (!score_list || !name)
        return NULL;

    if (!*score_list) // Caso lista vuota
    {
        Score * tmp = scoreboard_newScore(name, 0);
        if (!tmp)
            return NULL;

        scoreboard->serialized[ scoreboard_serialize_index(scoreboard, index, topic) ].modified = true; // Rifaccio la serializzazione
        
        return listDoubly_append(score_list, tmp);
    }

    // Caso elemento già in lista
    DNode *current = *score_list;
    for (; current && current->next; current = current->next)
    {
        char * score_name = ((Score *) current->data)->name;
        if ( score_name && !strcmp( score_name , name) )
            return current;
    }
    
    // Caso elemento non trovato
    // Evito di ripercorrere tutta la intera lista
    // Aggiungo in fondo perchè è probabile che il punteggio sia 0 inizialmente
    DNode *ret;
    Score * tmp = scoreboard_newScore(name, 0);
    if (!tmp)
        return NULL;
    if (!(ret = listDoubly_append(&current, tmp)))
        free(tmp);

    scoreboard->serialized[ scoreboard_serialize_index(scoreboard, index, topic) ].modified = true; // Rifaccio la serializzazione

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

        for (int t = 0; t < scoreboard->nTopics; t++)
        {
            SerializedScoreboard *current = &scoreboard->serialized[ scoreboard_serialize_index(scoreboard, i, t) ];
            if (current->modified)
            {
                char buffer[512];

                #ifdef CUSTOM_PRINT
                    if (i == SCR_PLAYING)
                        snprintf(buffer, sizeof(buffer), "Punteggio tema \"%d - %s\"\n", t + 1, topics->topics[t].name);
                    else
                        snprintf(buffer, sizeof(buffer), "Quiz tema \"%d - %s\" completato\n", t + 1, topics->topics[t].name);
                #else
                    if (i == SCR_PLAYING)
                        snprintf(buffer, sizeof(buffer), "Punteggio tema %d\n", t + 1);
                    else
                        snprintf(buffer, sizeof(buffer), "Quiz tema %d completato\n", t + 1);
                #endif
                
                // Dimensione della stringa
                current->serialized_lenght = strcpyResize(&current->string, 
                                                            buffer, 
                                                            &current->serialized_allocated, 
                                                            0);

                // Punteggi effettivi: scorro tutta la lista
                for (DNode * node = scoreboard->scores[i][t]; node; node = node->next)
                {
                    Score *score = node->data;

                    #ifdef CUSTOM_PRINT
                        snprintf(buffer, sizeof(buffer), "- %s: %d\n", score->name, score->score);
                    #else
                        snprintf(buffer, sizeof(buffer), "- %s %d\n", score->name, score->score);
                    #endif

                    // Dimensione della stringa
                    current->serialized_lenght += strcpyResize(&current->string, 
                                                                buffer, 
                                                                &current->serialized_allocated, 
                                                                current->serialized_lenght);
                }

                // current->serialized_lenght[t]++; // The last NULL caracher, rimosso perché è più semplice ragionare senza il carattere nullo
                current->modified = false;
            }
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

void scoreboard_print(Scoreboard *scoreboard) // solo di DEBUG, usare le serializzazioni
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

void scoreboard_scorePrint(void *score) // solo di DEBUG, usare le serializzazioni
{
    printf("- %s: %d", ((Score *) score)->name, ((Score *) score)->score);
}

// incremento il punteggio e riordino la lista (O(n))
void scoreboard_increaseScore(Scoreboard * scoreboard, DNode * score, int topic)
{
    ((Score*) score->data)->score++;
     
    listDoubly_sortElement( &scoreboard->scores[SCR_PLAYING][topic],
                            NULL, score, scoreboard_scoreCompare);
    
    scoreboard->serialized[ scoreboard_serialize_index(scoreboard, SCR_PLAYING, topic) ].modified = true;
}

// Estraggo il nodo e lo sposto nell'altra lista semplicemente
bool scoreboard_completedScore(Scoreboard *scoreboard, DNode * elem, int topic)
{
    if (!scoreboard || !elem || topic < 0)
        return false;

    DNode *tmp = listDoubly_DNode_extract( &scoreboard->scores[SCR_PLAYING][topic], NULL, elem);

    if (!tmp)
        return false;

    scoreboard->serialized[ scoreboard_serialize_index(scoreboard, SCR_PLAYING, topic) ].modified = true;

    if (!listDoubly_DNode_insert( &scoreboard->scores[SCR_COMPLETED][topic], tmp, scoreboard_scoreCompare))
        return false;

    scoreboard->serialized[ scoreboard_serialize_index(scoreboard, SCR_COMPLETED, topic) ].modified = true;

    return true;
}

// estraggo il nodo e lo dealloco
void scoreboard_removeScore(Scoreboard * scoreboard, DNode * score, int index, int topic)
{
    if (topic < 0 || topic >= scoreboard->nTopics)
        return;

    if (!scoreboard || !score)
        return;

    DNode * tmp = listDoubly_DNode_extract(&scoreboard->scores[index][topic], NULL, score);

    if (!tmp)
        return;

    scoreboard->serialized[ scoreboard_serialize_index(scoreboard, index, topic) ].modified = true;
    scoreboard_scoreDestroy(tmp->data);
    free(tmp);
}