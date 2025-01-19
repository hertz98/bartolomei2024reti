#pragma once
#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"
#include "../shared/message.h"
#include "topic.h"

typedef enum OperationResult {
    OP_FAIL = false, // L'operazione non ha avuto successo
    OP_OK = true, // L'operazione sta procedendo
    OP_DONE // L'operazione è terminata con successo
} OperationResult;

typedef struct Client Client;
typedef struct ClientsContext ClientsContext;
typedef struct Message Message;

struct Client
{
    bool registered;
    char * name;

    struct Operation
    {
        OperationResult (* operationHandler)(ClientsContext *context, int socket, void *, OperationResult (* operation)(ClientsContext * context, int socket, void * buffer));
        OperationResult (* operation)(ClientsContext *context, int socket, void *);
        // Parametri con cui verrà richiamata la funzione operationHandler
        int step;
        void * p;
    } operation;
    
    MessageArray * toSend; // Messaggi
    int transferring;

    // Topics
    struct Game{
        int playing; // indice topic corrente
        bool * playableTopics; // array di topic giocabili dal Client
        int * score; // array di punteggi 
        int currentQuestion; // indice domanda corrente
        Question * questions; // array di domande (allo scopo di mischiarle)
    } game;
};

struct ClientsContext {
    int nClients; // Numero di clients attuali
    int maxClients; // Numero massimo di clients servibili

    fd_set master_fds,
           read_fds,
           write_fds;
    int fd_max; // Massimo intero nel set master
    int listener;

    Client ** clients;
    int allocated; // Numero di strutture allocate per i clients
};

int clientsInit(ClientsContext * clientsContext, int max);

bool clientAdd(ClientsContext * context, int socket);
void clientRemove(ClientsContext * context, int socket);
void clientsFree(ClientsContext *context);

bool isClient(ClientsContext *context, int socket, bool onlyRegistered);

bool sendCommand(int socket, enum Command);
enum Command recvCommand(int socket);

OperationResult sendMessageHandler(ClientsContext * context, int socket);

OperationResult sendData(int socket, void * buffer, unsigned int lenght, unsigned int * sent);
OperationResult recvData(int socket, void *buffer, unsigned int lenght, unsigned int *received);

bool nameValid(ClientsContext * context, int socket, char * name);

bool gameInit(Client * clientsContext, TopicsContext * topicsContext);

OperationResult sendTopics(ClientsContext *context, int socket, void *, bool init);

// OperationHandlers

OperationResult confirmedOperation(ClientsContext * context, int socket, void *, OperationResult (* operation)(ClientsContext * context, int socket, void * buffer));

OperationResult giveConfirmOperation(ClientsContext * context, int socket, void *, OperationResult (* operation)(ClientsContext * context, int socket, void * buffer));

// Operations

OperationResult sendMessage(ClientsContext * context, int socket, void * message);

OperationResult recvMessage(ClientsContext * context, int socket, void * pointer);
