#include <stdbool.h>
#include <linux/limits.h>
#include "list.h"

#ifndef PATH_BUFFER_SIZE
#define PATH_BUFFER_SIZE 4096
#endif

typedef struct Question 
{
    char * question;
    char * answer;
} Question;

typedef struct Topic 
{
    char name[NAME_MAX];
    Node * questions;
} Topic;

typedef struct TopicsContext
{
    char directory[PATH_MAX];
    int nTopics;
    struct Topic * topics;
} TopicsContext;

/// @brief Inizializza le strutture dati inerrenti ai topic
/// @param context Strutture dati inerenti ai topic non inizializzate
/// @param directory Percorso contenente i topic
/// @return Ritorna true se ha successo
bool topicsInit(TopicsContext *context, char * directory);

/// @brief Analizza la cartella dei topic e per ciascun file crea un topic e lo carica in memoria
/// @param context Strutture dati inerenti ai topic inizializzate
/// @return Ritorna true se fila tutto liscio
bool topicsLoader(TopicsContext *context);

/// @brief Dato il topic si occupa di caricarne domande e risposte nella lista
/// @param context Strutture dati inerenti ai topic inizializzate
/// @param topic Struttura dati associata al topic contenente il nome del file (completo)
/// @return Ritorna true se fila tutto liscio
bool topicLoad(TopicsContext *context, struct Topic * topic);

/// @brief Si occupa di deallocare tutto ciò che riguarda i topic
/// @param context Strutture dati inerenti ai topic inizializzate
void topicsFree(TopicsContext *context);

/// @brief Dato l'utente determina a quale dei topic attualmente disponibili abbia effettivamente giocato O(n^2)
/// @param context Strutture dati inerenti ai topic inizializzate
/// @param user Username dell'utente (stringa)
/// @return Ritorna un array di booleani di lunghezza nTopics, dove true significa giocabile
bool * topicsGet(TopicsContext *context, char * user);

/// @brief Si occupa di contrassegnare su disco un topic come giocato per un certo l'utente
/// @param context Strutture dati inerenti ai topic inizializzate 
/// @param user Username dell'utente (stringa) 
/// @param topic Topic da contrassegnare come giocato (stringa)
/// @return Ritorna true se ha successo
bool topicSet(TopicsContext * context, char * topic, char * user);

/* FUNZIONI UTILITA' */

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