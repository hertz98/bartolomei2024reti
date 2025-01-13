#pragma once
#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"
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

    // Status
    OperationResult (* longOperation)(ClientsContext * client, int socket, void *, bool init);
    int step;

    int tmp_i, tmp_i2;
    void * tmp_p, * tmp_p2;
    
    Message * sending; // Puntatore al messaggio da inviare
    void (* sendHandler)(ClientsContext * context, int socket);

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

    fd_set master,
           readSet,
           writeSet;
    int fd_max; // Massimo intero nel set master
    int listener;

    Client ** clients;
    int allocated; // Numero di strutture allocate per i clients
};

struct Message {
    void * data; // Puntatore al contenuto del messaggio da inviare
    int lenght; // Dimensione del messaggio (compreso di caratteri speciali)
    int sent; // Numero di bytes inviati
    bool toFree; // Indica se data deve essere deallocato dopo l'invio
    struct Message *next; // Prossimo messaggio (se esiste)
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

/// @brief Prepara la struttura dati di un messaggio contenente una stringa
/// @param string Stringa con carattere di terminazione nullo
/// @param toFree se true, la stringa deve essere deallocata alla distruzione della lista
/// @return Ritorna un nodo di una lista di messaggi
Message * messageString(const char * string, bool toFree);

OperationResult sendMessage(ClientsContext *context, int socket, void * buffer, bool init);
bool sendString(int socket, char *, int);

OperationResult recvMessage(ClientsContext *context, int socket, void * buffer, bool init);
bool recvString(int socket, char **, int);

OperationResult regPlayer(ClientsContext *context, int socket, void *, bool init);
bool nameValid(ClientsContext * context, int socket, char * name);

bool gameInit(Client * clientsContext, TopicsContext * topicsContext);

OperationResult sendTopics(ClientsContext *context, int socket, void *, bool init);