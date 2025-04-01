#pragma once

#ifndef SCOREBOARD_HEADER
#define SCOREBOARD_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>
#include "topic.h"

#include "../shared/doubly_list.h"

//#define SCOREBOARD_SAVE_SCORE
#define PRINT_TOPIC_NAMES_ALWAYS

#define DEFAULT_SCOREBOARD_ALLOCATION 4096
#define SCOREBOARD_SIZE 2
enum ScoreboardType {SCR_CURRENT, SCR_COMPLETED};

typedef struct Score
{
    char *name;
    int score;
} Score;

typedef struct SerializedScoreboard
{
    bool modified; // This is the only optimization, serialize only the modified topics
    char * string;
    int serialized_lenght; // The size of the string
    int serialized_allocated; // The allocated size
} SerializedScoreboard;

typedef struct Scoreboard
{
    int nTopics;

    DNode ** scores[ SCOREBOARD_SIZE ]; // Array di liste a Score
    SerializedScoreboard * serialized;;
} Scoreboard;

/// @brief Inizializza le strutture dati inerenti ai punteggi
/// @param scoreboard Strutture dati inerenti ai punteggi non inizializzate
/// @param topisc Strutture dati inerenti ai topics finalizzate
/// @return true in caso di successo, false in caso di fallimento
bool scoreboard_init(Scoreboard *scoreboard, TopicsContext * topics);

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
/// @return +1 se a < b, uguale a 0 se a == b, -1 se a > 0 (inverso)
int scoreboard_scoreCompare(void *a_ptr, void *b_ptr);

/// @brief Funzione per stampare la classifica (debug)
/// @param scoreboard Puntatore a struttura dati contenente la classifica
void scoreboard_print(Scoreboard * scoreboard);

/// @brief Funzione per stampare un punteggio (debug)
/// @param score Puntatore a punteggio
void scoreboard_scorePrint(void * score);

/// @brief Alloca un nuovo Score
/// @param name puntatore al nome del giocatore
/// @param score punteggio
/// @return il puntatore allo Score allocato o NULL
Score * scoreboard_newScore(char * name, int score);

/// @brief Inizializza le strutture dati che rappresentano la classifica serializzata
/// @param scoreboard Il puntatore alla struttura dati inerenti alla classifica
/// @return true in caso di successo, false altrimenti
bool scoreboard_serialize_init(SerializedScoreboard * scoreboard);

/// @brief Aggiorna le serializzazioni della classifica se necessario
/// @param scoreboard Puntatore a struttura dati contenente la classifica
void scoreboard_serialize_update(Scoreboard *scoreboard, TopicsContext * topics);

/// @brief Tratta un array continuo come foesse una matrice
/// @param scoreboard Puntatore alla struttura dati inerente alla classifica
/// @param index Indice della serialized scoreboard
/// @param topic Indice del topic 
/// @return L'indice rispetto all'array 1D
static inline int scoreboard_serialize_index(Scoreboard *scoreboard, int index, int topic)
{
    return index * scoreboard->nTopics + topic;
}

#endif