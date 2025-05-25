/* MESSAGE.H
 * contiene definizioni di strutture e funzioni per la standarizzare la trasmissione dei dati
*/

#pragma once

#ifndef MESSAGE_HEADER
#define MESSAGE_HEADER

#include <inttypes.h>
#include "list.h"

#define TCP_MAX_PAYLOAD 65535 // Non voglio confrontarmi con messaggi più grandi di quello che può gestire TCP

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
     // If false every message needs to be fully sent to return, even if the socket would block
     // This is necessary for data that can change on the half of sending
    bool isInterruptible;
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

/// @brief Crea un nuovo messageArray con gli stessi messaggi
/// @param  toCopy MessageArray da copiare
/// @return Ritorna una nuova copia allocata di un messageArray oppure NULL in caso di fallimento
MessageArray * MessageArrayCpy(MessageArray * toCopy);

/// @brief Resetta lo stato di trasmissione di un intero MessageArray
/// @param toReset Il MessageArray
void messageArray_reset(MessageArray * toReset);

/// @brief restituisce un array di puntatori a stringhe da un MessageArray
/// @param messageArray messageArray contenente solo stringhe
/// @return un puntatore a puntatori a stringa
char ** messageArray2StringArray(MessageArray * messageArray);

/// @brief Prepara la struttura dati di un messaggio per contenere una stringa
/// @param message l'indirizzo del messaggio
/// @param string Stringa con carattere di terminazione nullo
/// @param toFree se true, la stringa deve essere deallocata alla distruzione della lista
void messageString(Message *message, char *string, bool toFree);

/// @brief Prepara un messaggio di dimensione già conosciuta (evita l'overhead di dover ricontare la dimensione della stringa)
/// @param message l'indirizzo del messaggio
/// @param string Stringa con carattere di terminazione nullo
/// @param size Il numero di caratteri della stringa, escluso il carattere di terminazione nullo
/// @param toFree se true, la stringa deve essere deallocata alla distruzione della lista
void messageStringReady(Message *message, char *string, int size, bool toFree);

/// @brief Prepara la struttura dati di un messaggio contenente un array di interi
/// @param message l'indirizzo del messaggio
/// @param number Intero da mandare
/// @return true, false in caso di fallimento
bool messageInteger(Message * message, int32_t number);

/// @brief Prepara la struttura dati di un messaggio per contenrere un array di booleani
/// @param message l'indirizzo del messaggio
/// @param array indirizzo all'array di booleani
/// @param size Dimensione dell'array
void messageBoolArray(Message * message, bool *array, int size);

/// @brief Prepara la struttura dati di un messaggio per contenrere un array di booleani
/// @param message l'indirizzo del messaggio
/// @param array indirizzo all'array di booleani
/// @param size Dimensione dell'array
/// @return true, false in caso di fallimento
bool messageIntegerArray(Message *message, int32_t *array, int size);

/// @brief Azzera il contenuto di un messaggio
/// @param message Indirizzo del messaggio
void emptyMessage(Message *message);

#endif