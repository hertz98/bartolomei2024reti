/* TOPIC.H
 * Le strutture e le funzioni dei topic organizzano in memoria le domande e le risposte suddivisi per topic
 * Le funzioni in questo header sono quelle a più stretto contatto con il file system, in particolare si
 * caricanno in memoria i topics, si carica e si salva lo stato di gioco dei players
 * 
 * TopicsContext/
 * ├── directory [PATH_MAX]
 * ├── nTopics
 * ├── topicsString [nTopics]
 * └── topics [nTopics]
 *     ├── *questions
 *     ├── nQuestions
 *     └── name [NAME_MAX]
 */

#pragma once

#ifndef TOPIC_HEADER
#define TOPIC_HEADER

#include <stdbool.h>
#include <limits.h>

#include "../parameters.h"
#include "../shared/message.h"
#include "../shared/list.h"

typedef struct Question 
{
    char * question;
    char * answer;
} Question;

typedef struct Topic 
{
    char name[NAME_MAX];
    int nQuestions;
    Node * questions;
} Topic;

typedef struct TopicsContext
{
    char directory[PATH_MAX];
    int nTopics;
    struct Topic * topics;
    MessageArray * topicsString; // Cache per inviare la lista dei topics
} TopicsContext;

/// @brief Inizializza le strutture dati inerrenti ai topic
/// @param context Strutture dati inerenti ai topic non inizializzate
/// @param directory Percorso contenente i topic
/// @return Ritorna true in caso di successo, false altrimenti
bool topicsInit(TopicsContext *context);

/// @brief Analizza la cartella dei topic e per ciascun file crea un topic e lo carica in memoria
/// @param context Strutture dati inerenti ai topic inizializzate
/// @return Ritorna true se fila tutto liscio, false altrimenti
bool topicsLoader(TopicsContext *context);

/// @brief Dato il topic si occupa di caricarne domande e risposte nella lista
/// @param context Strutture dati inerenti ai topic inizializzate
/// @param path La directory che contiene i topics
/// @return Ritorna true se fila tutto liscio
bool topicLoad(char * path, struct Topic * topic);

/// @brief Si occupa di deallocare tutto ciò che riguarda i topic
/// @param context Strutture dati inerenti ai topic inizializzate
void topicsFree(TopicsContext *context);

/// @brief Dato l'utente determina a quale dei topic attualmente disponibili
/// abbia effettivamente giocato O(n^2)
/// @param context Strutture dati inerenti ai topic inizializzate
/// @param user Username dell'utente (stringa)
/// @return Ritorna un array di booleani di lunghezza nTopics, dove true significa giocabile
bool * topicsUnplayed(TopicsContext *context, char * user);

/// @brief In maniera del tutto simile (ma inversa) a topicsUnplayed, dato l'utente
/// oltre a determinare quali topic sono stati giocati ne determina anche il punteggio se disponibile
/// @param context Strutture dati inerenti ai topic inizializzate
/// @param user Username dell'utente (stringa)
/// @return Ritorna un array di interi di lunghezza nTopics, dove i topics non giocati hanno
/// valore -1, mentre quelli giocati hanno il punteggio
int * topicsPlayed(TopicsContext *context, char *user);

/// @brief Si occupa di contrassegnare su disco un topic come giocato per un certo utente
/// @param context Strutture dati inerenti ai topic inizializzate 
/// @param user Username dell'utente (stringa) 
/// @param topic Topic da contrassegnare come giocato
/// @param score Il punteggio da registrare, -1 per non scriverlo
/// @return Ritorna true se ha successo
bool topicMakePlayed(TopicsContext * context, char * user, int topic, int score);

/* FUNZIONI UTILITA' */

/// @brief Crea una directory nel percorso voluto
/// @param buffer_path buffer contentente il percorso dove creare la directory
/// @param directory nome della directory
/// @return true se la directory è stata creata o esiste già, false in case di errore
bool directoryCreate(char * buffer_path, char * directory);

/// @brief Funzione utilità: dati due topic, compara i nomi dei topic al fine di permetterne il riordinamento usando qsort della stl
/// @param a Puntatore void al primo topic
/// @param b Puntatore void al secondo topic
/// @return Ritorna il risultato della strcmp tra stringhe dei nomi dei topic
int topics_compare(const void *a, const void *b);

/// @brief Funzione utilità: da utilizzare in list_print al fine di fare la print di domanda e risposta di ciascuna domanda nella lista
/// @param data Puntatore al dato del nodo, punta a una Question
void topic_list_print_question(void * data);

/// @brief Funzione utilità: da utilizzare in list_destroy, si occupa di deallocare domanda e risposta
/// @param data Puntatore al dato del nodo, punta a una Question
void topics_questionDestroy(void * data);

/// @brief Ottiene il nome del topic dal nome del file completo
/// @param name Stringa contenente il nome del file con numerazione ed estensione
void topic_name(char * name);

#endif