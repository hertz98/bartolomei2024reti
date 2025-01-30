#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_OPERATIONS_PER_CLIENT 5

// Limito le dimensioni del messaggi del client in base alle esigenze
#define RECV_MAX_MESSAGEARRAY_SIZE 1
#define RECV_MAX_MESSAGE_LENGHT 256

typedef struct ClientsContext ClientsContext;

// Operations

typedef enum OperationResult {
    OP_FAIL = false, // L'operazione non ha avuto successo
    OP_OK = true, // L'operazione sta procedendo
    OP_DONE // L'operazione è terminata con successo
} OperationResult;

typedef struct Operation
{
    OperationResult (* function)(ClientsContext *context, int socket, void *);
    // Parametri con cui verrà richiamata la funzione
    int step;
    void * p;

    void *tmp;
} Operation;

/// @brief Si occupa di gestire e eseguire le operazioni in ordine e di deallocarle al termine
/// @param client Struttura dati del client inizializzata
/// @return Il risultato dell'operazione, false in caso di fallimento
bool operationHandler(ClientsContext *context, int socket);

/// @brief Verifica se un socket ha ulteriori dati disponibili
/// @param socket Indice del descrittore del socket
/// @return true se il socket ha dati disponibili, false altrimenti (o in caso di errore)
bool socketReady(int socket);

/// @brief Alloca una nuova operazione e la esegue
/// @param function Operazione da eseguire
/// @param context Strutture dati inerenti ai client inizializzate da passare alla operazione
/// @param socket Indice del socket da passare alla operazione
/// @param p Puntatore da passare alla operazione
/// @return Il risultato dell'operazione, false in caso di fallimento
bool operationCreate(OperationResult (* function)(ClientsContext *context, int socket, void *), ClientsContext *context, int socket, void *p);

/// @brief Distrugge le strutture dati della Operation
/// @param operation indirizzo della Operation
void operationDestroy(void * operation);

/// @brief Operation sendMessage, invia uno o più messaggi e aspetta conferma
/// @param context Strutture dati inerrenti ai client inizializzate
/// @param socket Indice del socket del client
/// @param message Struttura MessageArray pronta contenente il/i dati da inviare
/// @return Ritorna l'esito dell'operazione, false in caso di fallimento
OperationResult sendMessage(ClientsContext * context, int socket, void * message);

/// @brief Operation recvMessage, riceve uno o più messagi e inivia conferma di avvenuta ricezione
/// @param context Strutture dati inerrenti ai client inizializzate
/// @param socket Indice del socket del client
/// @param pointer Indirizzo dove il MessageArray verrà allocato
/// @return Ritorna l'esito dell'operazione, false in caso di fallimento
OperationResult recvMessage(ClientsContext * context, int socket, void * pointer);

/// @brief Operation regPlayer, riceve il nome del client e verifica se accettabile
/// @param context Strutture dati inerrenti ai client inizializzate
/// @param socket Indice del socket del client
/// @param topicsContext Strutture dati inerenti ai topic finalizzate
/// @return Ritorna l'esito dell'operazione, false in caso di fallimento
OperationResult regPlayer(ClientsContext * context, int socket, void * topicsContext);

/// @brief Invia le informazioni dei topics al client, e verifica la risposta
/// @param context Strutture dati inerrenti ai client inizializzate
/// @param socket Indice del socket del client
/// @param topicsContext Strutture dati inerenti ai topic finalizzate
/// @return Ritorna l'esito dell'operazione, false in caso di fallimento
OperationResult selectTopic(ClientsContext *context, int socket, void * topicsContext);

/// @brief Se il topic è stato scelto, la Operation playTopic si occupa di inviare la domanda e verificare la risposta
/// @param context Strutture dati inerrenti ai client inizializzate
/// @param socket Indice del socket del client
/// @param topicsContext Strutture dati inerenti ai topic finalizzate
/// @return Ritorna l'esito dell'operazione, false in caso di fallimento
OperationResult playTopic(ClientsContext *context, int socket, void * topicsContext);