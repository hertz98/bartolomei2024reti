#pragma once
#include <inttypes.h>
#include "list.h"

/// @brief Un Message è una struttura che contiene i dati grezzi e il loro stato di trasmissione
typedef struct Message {
    void * payload; // Puntatore al contenuto del messaggio da inviare
    uint32_t lenght; // Dimensione del messaggio (compreso di caratteri speciali)
    uint32_t transmitted; // Numero di bytes inviati
    bool toFree; // Indica se data deve essere deallocato dopo l'invio
} Message;

/// @brief Un MessageArray contiene `size + 1` messaggi, l'ultimo messaggio contiene
/// il numero di messaggi in modo da poterla inviare al client, è significativo per
/// la trasmissione ma non necessariamente per la ricezione.
/// Ho scelto l'ultimo (anche se inviato per primo) per non cambiare l'indirizzamento 
/// al di fuori delle funzioni di trasmissione e ricezione.
typedef struct MessageArray {
    int size;
    Message * messages;
} MessageArray;

/// @brief Dealloca un MessageArray e il suo contenuto
/// @param messageArray Indirizzo del messageArray 
/// @param destroyer funzione per deallocare il payload dei messaggi in caso di strutture complicate
void messageArrayDestroy(MessageArray **messageArray);

/// @brief Prepara un nuovo MessageArray contenente lo spazio per `size + 1` messaggi
/// @param size Numero di messaggi
/// @return l'indirizzo del nuovo MessageArray appena allocato o NULL in caso di fallimento
MessageArray * messageArray(int size);

/// @brief Prepara la struttura dati di un messaggio contenente una stringa
/// @param message l'indirizzo del messaggio da 
/// @param string Stringa con carattere di terminazione nullo
/// @param toFree se true, la stringa deve essere deallocata alla distruzione della lista
void messageString(Message *message, char *string, bool toFree);

void messageInteger(Message * message, int32_t number);

void emptyMessage(Message *message);