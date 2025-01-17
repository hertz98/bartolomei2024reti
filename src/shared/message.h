#pragma once
#include <inttypes.h>
#include "list.h"

typedef struct Message {
    void * data; // Puntatore al contenuto del messaggio da inviare
    uint32_t lenght; // Dimensione del messaggio (compreso di caratteri speciali)
    uint32_t transmitted; // Numero di bytes inviati
    bool toFree; // Indica se data deve essere deallocato dopo l'invio
    struct Message *next; // Prossimo messaggio (se esiste)
} Message;

/// @brief Prepara la struttura dati di un messaggio contenente una stringa
/// @param string Stringa con carattere di terminazione nullo
/// @param toFree se true, la stringa deve essere deallocata alla distruzione della lista
/// @return Ritorna un nodo di una lista di messaggi
Message * messageString(char * string, bool toFree);

Message * emptyMessage();

