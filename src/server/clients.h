/* CLIENTS.H
 * Contiene funzioni e strutture dati inerenti ai clients e la comunicazione con essi
 * tra cui funzioni per la gestione dei clients, e della classifica
 * 
 * I clients vengono identificati tramite il loro indice di socket perciò la grandezza del
 * array dei clients dipende dagli indici assegnati dal sistema operativo ai socket
 * 
 * ClientsContext/
 * ├── nClients
 * ├── registered
 * ├── maxClients
 * ├── sending
 * ├── master_fds
 * ├── read_fds
 * ├── write_fds
 * ├── fd_max
 * ├── listener
 * ├── allocated 
 * ├── scoreboard   // Scoreboard.h
 * └── *clients [allocated]
 *     ├── *name
 *     ├── *operation
 *     ├── nOperations
 *     ├── registered
 *     ├── sending
 *     └── game
 *         ├── playing
 *         ├── playableTopics [topicsContext->nTopics]
 *         ├── *score [topicsContext->nTopics]
 *         ├── currentQuestion
 *         └── *questions [currentTopic->nQuestions]
*/

#pragma once

#ifndef CLIENTS_HEADER
#define CLIENTS_HEADER

#include <time.h>
#include <stdbool.h>
#include <sys/select.h>

#include "../parameters.h"
#include "../shared/commands.h"
#include "../shared/message.h"
#include "../shared/doubly_list.h"

#include "topic.h"
#include "operation.h"
#include "scoreboard.h"

typedef struct Client Client;
typedef struct ClientsContext ClientsContext;

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
        DNode ** score; // array di punteggi 
        int currentQuestion; // indice domanda corrente
        Question ** questions; // array di domande (allo scopo di mischiarle)
    } game;
};

struct ClientsContext {
    int nClients; // Numero di clients attuali
    int registered; // Numero di client attualmente registrati
    int maxClients; // Numero massimo di clients servibili

    int sending; // Numero di client a cui stiamo inviando dati

    fd_set master_fds,
           read_fds,
           write_fds;
    int fd_max; // Massimo intero nel set master
    int listener;

    Client ** clients; // Array dinamico dei puntatori a Client
    int allocated; // Numero di strutture allocate per i clients

    Scoreboard scoreboard;
};

/// @brief Inizializza il contesto dei Client
/// @param clientsContext Puntatore alle strutture dati inerenti ai clients non inizializzate
/// @param max Massimo numero di clients
/// @return true in caso di successo, false altrimenti
int clientsInit(ClientsContext * clientsContext, int max);

/// @brief Crea le strutture dati di un client appena accettato
/// @param context Puntatore alle strutture dati inerenti ai clients
/// @param socket socket corrispondente al client
/// @return true in caso di successo, false altrimenti
bool clientAdd(ClientsContext * context, int socket);

/// @brief Rimuove un client ed eventuali sotto strutture dati
/// @param context Puntatore alle strutture dati inerenti ai clients
/// @param topics Strutture dati inerenti ai topics
/// @param socket Socket corrispondente al client
void clientRemove(ClientsContext * context, TopicsContext * topics, int socket);

/// @brief Rimuove tutti i clients e il contesto
/// @param context Puntatore alle strutture dati inerenti ai clients
void clientsFree(ClientsContext *context, TopicsContext * topics);

/// @brief Indica se un certo indice di socket corrisponde ad un client valido
/// @param context Puntatore alle strutture dati inerenti ai clients
/// @param socket Socket corrispondente al client
/// @param onlyRegistered Aggiunge il requisito che il client deve avere un nome
/// @return true se rispetta le condizioni, false altrimenti
bool isClient(ClientsContext *context, int socket, bool onlyRegistered);

/// @brief Funzione bloccante, invia un comando di un singolo byte al client
/// @param socket Socket corrispondente al client
/// @return true se ha avuto successo, false altrimenti
bool sendCommand(int socket, enum Command);

/// @brief Riceve un comando dal client
/// @param socket Socket corrispondente al client
/// @return Il comando ricevuto o False
Command recvCommand(int socket);

/// @brief Funzione che invia dati, gestisce la send
/// @param socket Indice del descrittore di socket del client
/// @param buffer Buffer contenente i dati grezzi
/// @param lenght Numero di byte da inviare
/// @param sent Numero di byte già inviati
/// @param block L'operazione diventa bloccante finché tutti i byte non saranno inviati
/// @return OP_DONE se l'invio è avvenuto completamente, 
/// OP_OK se sarà necessario richiamare la funzione per terminare l'invio
/// OP_FAIL in caso di errore
OperationResult sendData(int socket, void * buffer, unsigned int lenght, unsigned int * sent, bool block);

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
/// @param context Struttura dati del client inizializzate con dati di gioco inizializzata
/// @param socket Indice del descrittore del socket corrispettivo al client
/// @param topicsContext Strutture dati inerenti ai topics
/// @return true se è andato tutto liscio
bool client_gameInit(ClientsContext * context, int socket, TopicsContext * topicsContext);

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

/// @brief Setta un topic come giocato sia a livello di strutture dati che su disco
/// @param client Struttura dati del client corrispondente con dati di gioco inizializzati
/// @param topics Strutture dati inerenti ai topics finalizzate
/// @param topic Indice reale del topic
/// @param write Scrivi il punteggio su disco
/// @return true se è andato tutto liscio
bool client_setPlayed(Client * client, TopicsContext * topics, int topic, bool writeScore);

/// @brief Invia l'intera lista dei topics al client e quali giocabili
/// @param context Struttura dati inerenti ai client inizializzate
/// @param socket Indice del descrittore del socket del client
/// @param topics Strutture dati inerenti ai topics finalizzate
OperationResult client_sendTopics(ClientsContext *context, int socket, TopicsContext * topics);
/// @return Ritorna l'esito dell'operazione creata

/// @brief Verifica se un socket ha ulteriori dati disponibili
/// @param socket Indice del descrittore del socket
/// @param timeout timeout della select
/// @return true se il socket ha dati disponibili, false altrimenti (o in caso di errore)
bool client_socketReady(int socket, struct timeval * timeout);

/// @brief Verifica se è possibile scrivere in un socket 
/// @param socket Indice del descrittore del socket
/// @param timeout timeout della select
/// @return true se il socket è pronto, false altrimenti (o in caso di errore)
bool client_socketWriteReady(int socket, struct timeval *timeout);

/// @brief Invia la classifica ai client
/// @param context Struttura dati inerenti ai client inizializzate
/// @param socket Indice del descrittore del socket del client
/// @return Ritorna l'esito dell'operazione creata
OperationResult client_sendScoreboard(ClientsContext *context, int socket);

/// @brief Invia un array di booleani che indica quali topics sono stati giocati
/// @param context Strutture dati inerenti ai client inizializzate
/// @param topics Strutture dati inerenti ai topics inizializzate
/// @param socket Indice del descrittore del socket del client
/// @return Ritorna l'esito dell'operazione creata
OperationResult client_sendPlayable(ClientsContext *context, TopicsContext * topics, int socket);

/// @brief Imposta il quiz di un client come terminato e salva il punteggio
/// @param context Strutture dati inerenti ai client inizializzate
/// @param socket Indice del descrittore del socket del client
/// @param topicsContext Strutture dati inerenti ai topics inizializzate
void client_endquiz(ClientsContext *context, int socket, TopicsContext *topicsContext);

#endif