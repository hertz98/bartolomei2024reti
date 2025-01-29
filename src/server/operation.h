#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

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

/// @brief Si occupa di eseguire le operazioni in ordine e di deallocarle al termine
/// @param client Struttura dati del client inizializzata
/// @return Il risultato dell'operazione, false in caso di fallimento
bool operationHandler(ClientsContext *context, int socket);

/// @brief Alloca una nuova operazione e la esegue
/// @param function Operazione da eseguire
/// @param context Strutture dati inerenti ai client inizializzate da passare alla operazione
/// @param socket indice del socket da passare alla operazione
/// @param p Puntatore da passare alla operazione
/// @return Il risultato dell'operazione, false in caso di fallimento
bool operationCreate(OperationResult (* function)(ClientsContext *context, int socket, void *), ClientsContext *context, int socket, void *p);

void operationDestroy(void * operation);

OperationResult sendMessage(ClientsContext * context, int socket, void * message);

OperationResult recvMessage(ClientsContext * context, int socket, void * pointer);

OperationResult regPlayer(ClientsContext * context, int socket, void * topicsContext);

OperationResult selectTopic(ClientsContext *context, int socket, void * topicsContext);

OperationResult playTopic(ClientsContext *context, int socket, void * topicsContext);