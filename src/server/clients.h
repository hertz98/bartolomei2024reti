#pragma once
#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"
#include "../shared/message.h"

#include "topic.h"
#include "operation.h"

// Il nome non può essere più lungo di PATH_MAX - ESTENSIONE
#define CLIENT_NAME_MAX 32
#define CLIENT_NAME_MIN 4

typedef struct Client Client;
typedef struct ClientsContext ClientsContext;
typedef struct Message Message;

struct Client
{
    bool registered;
    char * name;

    Node * operation;
    int nOperations;

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

enum Command nameValid(ClientsContext * context, int socket, char * name);

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
