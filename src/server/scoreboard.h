#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>

#include "../shared/doubly_list.h"

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
    DNode ** score_list; // Array di liste a Score
} Scoreboard;

/// @brief Inizializza le strutture dati inerenti ai punteggi
/// @param scoreboard Strutture dati inerenti ai punteggi non inizializzate
/// @return true in caso di successo, false in caso di fallimento
bool scoreboard_init(Scoreboard * scoreboard, int topics);

/// @brief Libera dalla memoria le strutture dati allocate inerenti ai punteggi
/// @param scoreboard Strutture dati inerenti ai punteggi
void scoreboard_destroy(Scoreboard * scoreboard);

/// @brief Si occupa di deallocare le strutture dati inerenti ai punteggi
/// @param p Puntatore a Scoreboard da deallocare
void scoreboard_scoreDestroy(void * p);

/// @brief Verifica se nella lista di Score esiste un punteggio
// che identifica lo stesso client, in caso contrario lo crea 
/// @param score_list Lista di puntatori a Score
/// @return Il puntatore a Score trovato o quello appena allocato, NULL in caso di fallimento
DNode *scoreboard_get(DNode **score_list, char * name);

/// @brief Funzione che compara due Score
/// @param a_ptr Primo punteggio
/// @param b_ptr Secondo punteggio
/// @return -1 se a < b, uguale a 0 se a == b, 1 se a > 0
int scoreboard_scoreCompare(void *a_ptr, void *b_ptr);

/// @brief Funzione per stampare la classifica (debug)
/// @param scoreboard Puntatore a struttura dati contenente la classifica
void scoreboard_print(Scoreboard * scoreboard);

/// @brief Funzione per stampare un punteggio (debug)
/// @param score Puntatore a punteggio
void scoreboard_scorePrint(void * score);