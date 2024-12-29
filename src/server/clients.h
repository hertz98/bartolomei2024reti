#pragma once
#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"
#include "topic.h"

typedef enum OperationStatus {
    OP_FAIL = false,
    OP_OK = true,
    OP_DONE
} OperationStatus;

typedef struct Client Client;
typedef struct ClientsContext ClientsContext;

struct Client
{
    bool registered;
    char * name;

    // Status
    OperationStatus (* operation)(ClientsContext *, int, void *, bool);
    int step;

    int tmp_i, tmp_i2;
    void * tmp_p, * tmp_p2;
    
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

    fd_set master;
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

bool sendInteger(int socket, int);
int recvInteger(int socket);

OperationStatus sendMessage(ClientsContext *context, int socket, void * buffer, bool init);
bool sendString(int socket, char *, int);

OperationStatus recvMessage(ClientsContext *context, int socket, void * buffer, bool init);
bool recvString(int socket, char **, int);

OperationStatus regPlayer(ClientsContext *context, int socket, void *, bool init);
bool nameValid(ClientsContext * context, int socket, char * name);

bool gameInit(Client * clientsContext, TopicsContext * topicsContext);

OperationStatus sendTopics(ClientsContext *context, int socket, void *, bool init);