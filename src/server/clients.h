#pragma once
#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"
#include "../shared/message.h"
#include "topic.h"

#define MAX_STACKABLE_OPERATIONS 2

typedef enum OperationResult {
    OP_FAIL = false, // L'operazione non ha avuto successo
    OP_OK = true, // L'operazione sta procedendo
    OP_DONE // L'operazione è terminata con successo
} OperationResult;

typedef struct Client Client;
typedef struct ClientsContext ClientsContext;
typedef struct Message Message;
typedef struct Operation Operation;

struct Client
{
    bool registered;
    char * name;

    struct Operation
    {
        OperationResult (* function)(ClientsContext *context, int socket, void *);
        // Parametri con cui verrà richiamata la funzione
        int step;
        void * p;

        void *tmp;
    } operation[MAX_STACKABLE_OPERATIONS]; // Si possono accavallare al più due operazioni
    
    MessageArray * toSend; // Messaggi
    int transferring;

    // Topics
    struct Game{
        int playing; // indice topic corrente
        bool * playableTopics; // array di topic giocabili dal Client
        int nPlayable;
        int * score; // array di punteggi 
        int currentQuestion; // indice domanda corrente
        Question ** questions; // array di domande (allo scopo di mischiarle)
    } game;
};

struct ClientsContext {
    int nClients; // Numero di clients attuali
    int registered; // Numero di client attualmente registrati
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

/// @brief Inizializza le strutture dati inerenti al gioco (si fa dopo la registrazione)
/// @param client Struttura dati del client inizializzate con dati di gioco non inizializzati
/// @param topicsContext Strutture dati inerenti ai topics
/// @return true se è andato tutto liscio
bool client_gameInit(Client * client, TopicsContext * topicsContext);

bool client_quizInit(Client * client, TopicsContext * topicsContext);

/// @brief Converte l'indice del topic fornito dal client nell'indice corrispondente nell'array dei topic
/// La funzione è fatta per prendere direttamente in ingresso l'input del client, avendo dei controlli
/// @param client Struttura dati del client corrispondente con dati di gioco inizializzati
/// @param topics Strutture dati inerenti ai topics
/// @param playable Indice dei topics dal punto di vista del client
/// @return Indice dello stesso topic corrispondente nell'array dei topics
int client_playableIndex(Client * client, TopicsContext * topics, int playable);

/// @brief Determina un topic come giocato sia a livello di strutture dati che su disco
/// @param client Struttura dati del client corrispondente con dati di gioco inizializzati
/// @param topics Strutture dati inerenti ai topics
/// @param topic Indice reale del topic
/// @return true se è andato tutto liscio
bool client_setPlayed(Client * client, TopicsContext * topics, int topic);

// Operations

Operation *getOperation(Client * client, void * function);

OperationResult sendMessage(ClientsContext * context, int socket, void * message);

OperationResult recvMessage(ClientsContext * context, int socket, void * pointer);

OperationResult regPlayer(ClientsContext * context, int socket, void * topicsContext);

OperationResult selectTopic(ClientsContext *context, int socket, void * topicsContext);

OperationResult playTopic(ClientsContext *context, int socket, void * topicsContext);