#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>

#include "../shared/list.h"

//#define SCOREBOARD_SAVE_SCORE

typedef struct Score
{
    char *name;
    int score;
} Score;

typedef struct Scoreboard
{
    int nTopics;
    int *nElements;
    Node ** score_list; // Array di liste a Score
} Scoreboard;

/// @brief Inizializza le strutture dati inerenti ai punteggi
/// @param scoreboard Strutture dati inerenti ai punteggi non inizializzate
/// @return true in caso di successo, false in caso di fallimento
bool scoreboard_init(Scoreboard * scoreboard, int topics);

/// @brief Libera dalla memoria le strutture dati allocate inerenti ai punteggi
/// @param scoreboard Strutture dati inerenti ai punteggi
void scoreboard_destroy(Scoreboard * scoreboard);

void scoreboard_scoreDestroy(void * p);

/// @brief Verifica se nella lista di Score esiste un punteggio
// che identifica lo stesso client, in caso contrario lo crea 
/// @param score_list Lista di puntatori a Score
/// @return Il puntatore a Score trovato o quello appena allocato, NULL in caso di fallimento
Score *scoreboard_get(Node **score_list, char * name);

/// @brief Incrementa un punteggio
/// @param score Puntatore ad un punteggio
void scoreboard_increase(Score * score);