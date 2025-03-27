#pragma once
#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"
#include "../shared/message.h"
#include "../shared/doubly_list.h"

#include "topic.h"
#include "operation.h"
#include "scoreboard.h"

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

    Node * operation; // Lista di operazioni
    int nOperations; // Numero di operazioni attualmente acavallate
    bool sending; // Identifica se è in corso un'operazione di invio di dati

    // Topics
    struct Game{
        int playing; // indice topic corrente
        bool * playableTopics; // array di topic giocabili dal Client
        int nPlayable;
        DNode ** score; // array di punteggi 
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

    Scoreboard scoreboard;
};

int clientsInit(ClientsContext * clientsContext, int max);

bool clientAdd(ClientsContext * context, int socket);
void clientRemove(ClientsContext * context, int socket);
void clientsFree(ClientsContext *context);

bool isClient(ClientsContext *context, int socket, bool onlyRegistered);

bool sendCommand(int socket, enum Command);
enum Command recvCommand(int socket);

/// @brief Funzione che invia dati, gestisce la send
/// @param socket Indice del descrittore di socket del client
/// @param buffer Buffer contenente i dati grezzi
/// @param lenght Numero di byte da inviare
/// @param sent Numero di byte già inviati
/// @return OP_DONE se l'invio è avvenuto completamente, 
/// OP_OK se sarà necessario richiamare la funzione per terminare l'invio
/// OP_FAIL in caso di errore
OperationResult sendData(int socket, void * buffer, unsigned int lenght, unsigned int * sent);

/// @brief Funzione che riceve dati, gestisce la recv
/// @param socket Indice del descrittore di socket del client
/// @param buffer Buffer che conterrà i dati grezzi
/// @param lenght Numero di byte da ricevere (o dimensione del buffer)
/// @param sent Numero di byte già ricevuti
/// @return OP_DONE se la ricezione è avvenuta completamente, 
/// OP_OK se sarà necessario richiamare la funzione per terminare la ricezione
/// OP_FAIL in caso di disconnessione del client o di altro errore
OperationResult recvData(int socket, void *buffer, unsigned int lenght, unsigned int *received);

/// @brief Verifica il nome utente fornito dal client
/// @param context Struttura dati del client inizializzate con dati di gioco inizializzata
/// @param socket Indice del descrittore del socket corrispettivo al client
/// @param name Stringa contenente il nome fornito dal client
/// @return Il comando da inviare al client
Command nameValid(ClientsContext * context, int socket, char * name);

/// @brief Inizializza le strutture dati inerenti al gioco (si fa dopo la registrazione)
/// @param client Struttura dati del client inizializzate con dati di gioco non inizializzati
/// @param topicsContext Strutture dati inerenti ai topics
/// @return true se è andato tutto liscio
bool client_gameInit(Client * client, TopicsContext * topicsContext);

/// @brief Carica il topic nella struttura dati del client
/// @param client Struttura dati del client inizializzata con dati di gioco inizializzati
/// @param topicsContext Strutture dati inerenti ai topics
/// @return true se va tutto liscio
bool client_quizInit(ClientsContext * context, int socket, TopicsContext *topicsContext);

/// @brief (Non più usato) Converte l'indice del topic fornito dal client nell'indice corrispondente nell'array dei topic
/// La funzione è fatta per prendere direttamente in ingresso l'input del client, avendo dei controlli
/// @param client Struttura dati del client corrispondente con dati di gioco inizializzati
/// @param topics Strutture dati inerenti ai topics
/// @param playable Indice dei topics dal punto di vista del client
/// @return Indice dello stesso topic corrispondente nell'array dei topics
int client_playableIndex(Client * client, TopicsContext * topics, int playable);

/// @brief Verifica che l'indice del topic ritornato dal client sia ammissibile
/// @param client Struttura dati del client corrispondente con dati di gioco inizializzati
/// @param topics Strutture dati inerenti ai topics
/// @param playable Indice dei topics dal punto di vista del client
/// @return Indice dello stesso topic corrispondente nell'array dei topics
bool client_checkTopicIndex(Client * client, TopicsContext * topics, int playable);

/// @brief Determina un topic come giocato sia a livello di strutture dati che su disco
/// @param client Struttura dati del client corrispondente con dati di gioco inizializzati
/// @param topics Strutture dati inerenti ai topics finalizzate
/// @param topic Indice reale del topic
/// @return true se è andato tutto liscio
bool client_setPlayed(Client * client, TopicsContext * topics, int topic);

/// @brief Invia l'intera lista dei topics al client
/// @param context Struttura dati inerenti ai client inizializzate
/// @param socket Indice del descrittore del socket del client
/// @param topics Strutture dati inerenti ai topics finalizzate
/// @return true se è andato tutto liscio
bool client_sendTopics(ClientsContext *context, int socket, TopicsContext * topics);