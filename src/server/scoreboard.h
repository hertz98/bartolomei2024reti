/* SCOREBOARD.H
 * La scoreboard si occupa di mantenere i punteggi dei giocatori in maniera ordianta  
 * e distinta per ogni topic, oltre a mantenere una cache della serializzazione degli stessi
 */

#pragma once

#ifndef SCOREBOARD_HEADER
#define SCOREBOARD_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>
#include "topic.h"

#include "../shared/doubly_list.h"

#define SCOREBOARD_SIZE 2
enum ScoreboardType 
{
    SCR_PLAYING, // Rappresenta la classifica dei giocatori in gioco
    SCR_COMPLETED // Rappresenta la classifica dei giochi completati
};

typedef struct Score
{
    char *name; // Nome del giocatore
    int score; // Punteggio
} Score;

typedef struct SerializedScoreboard
{
    bool modified; // Unica ottimizzazione fatta: vengono aggiornati solo le serializzazioni modificate
    char * string; // Serializzazione della classifica corrispondente al topic
    int serialized_lenght; // Dimensione della serializzazione attuale
    int serialized_allocated; // Dimensione attualmente allocata dall'array dinamico
} SerializedScoreboard;

typedef struct Scoreboard
{
    int nTopics; // Numero di topics (ridondante ma utile)

    DNode ** scores[ SCOREBOARD_SIZE ]; // Array di liste a Score
    SerializedScoreboard * serialized;; // Array 1D delle serializzazioni
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
/// @param scoreboard Puntatore a strutture dati inerenti alla classifica
/// @param index Indice della classifica
/// @param topic Indice del topic
/// @param name Nome del giocatore
/// @return Un nodo di una lista contenente il punteggio desiderato o NULL in caso di fallimento
DNode *scoreboard_get(Scoreboard * scoreboard, int index, int topic, char * name);

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

/// @brief Incrementa un punteggio e ordina la classifica
/// @param scoreboard Puntatorte alla struttura dati inerente alla classifica
/// @param score Punteggio da incrementare
/// @param topic Indice del topic a cui appartiene il punteggio
void scoreboard_increaseScore(Scoreboard * scoreboard, DNode * score, int topic);

/// @brief Sposta il punteggio corrente in quelli completati
/// @param scoreboard Puntatorte alla struttura dati inerente alla classifica
/// @param elem Nodo corrispondente nella lista dei punteggi correnti
/// @param topic Indice del topic corrispondente
/// @return true se ha avuto successo
bool scoreboard_completedScore(Scoreboard *scoreboard, DNode * elem, int topic);

/// @brief Rimuove un punteggio dalla classifica corrispondente
/// @param scoreboard Puntatore alla struttura dati inerente alla classifica
/// @param score Puntatore al nodo contenente il punteggio
/// @param index Indice della classifica corrispondente
/// @param topic Indice del topic corrispondente
void scoreboard_removeScore(Scoreboard *scoreboard, DNode *score, int index, int topic);

#endif