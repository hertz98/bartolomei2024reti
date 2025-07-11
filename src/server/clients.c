#define _DEFAULT_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <sys/time.h>
#include "clients.h"
#include "operation.h"
#include "util.h"

int clientsInit(ClientsContext *context, int max)
{
    if (max < 0 || max > 1024)
        return false;

    memset(context, 0, sizeof(ClientsContext));

    context->nClients = 0;
    context->maxClients = max;
    
    context->clients = malloc( sizeof(Client *) * (max));
    if (!context->clients)
        return false;

    context->allocated = max;
    memset(context->clients, 0, context->allocated);
    FD_ZERO(&context->master_fds);
    FD_ZERO(&context->read_fds);
    FD_ZERO(&context->write_fds);

    context->sending = 0;

    return true;
}

bool clientAdd(ClientsContext * context, int socket)
{
    if (context->nClients >= context->maxClients)
    {
        sendCommand(socket, CMD_FULL);
        return false;
    }
    
    // Il numero di socket è usato per indirizzare le strutture dati
    // ma non necessariamente coincide col numero di clients
    const int increment = CLIENTs_ARRAY_INCREMENT;
    if (socket >= context->allocated) //Realloco l'array dei clients
    {
        Client **tmp = realloc(context->clients, sizeof(Client *) * (socket + increment));
        if (!tmp)
            return false;

        memset(tmp + context->allocated, 0, sizeof(Client *) * increment);

        context->allocated += increment;
        context->clients = tmp;
    }

    // Creo la struttura dati
    context->clients[socket] = malloc(sizeof(Client));
    if (!context->clients[socket])
        return false; // No need to clean, the pointer remains NULL
    
    Client * client = context->clients[socket];
    memset(client, 0, sizeof(Client));

    context->nClients++;
    if (socket > context->fd_max)
        context->fd_max = socket;

    client->name = NULL;
    client->registered = false;
    client->game.playableTopics = NULL;
    client->game.questions = NULL;
    client->game.score = NULL;
    client->operation = NULL;
    client->nOperations = 0;

    FD_SET(socket, &context->master_fds);

    // Il protocollo TCP mette a disposizione un'opzione per monitorare il socket
    // durante la inattività, in caso di mancata risposta per un certo periodo si
    // termina la connessione

    int keepalive = 1; // enable
    int keepidle = 60; // Tempo di inattività per testare la connessione
    int keepintvl = 10; // Intervallo tra i test
    int keepcnt = 5;  // Numero di tentativi
    if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0 ||
        setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) < 0 ||
        setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) < 0 ||
        setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt)) < 0
        )
    {
        perror("setsockopt(SO_KEEPALIVE) failed");
    }

    if (!sendCommand(socket, CMD_OK)) // Invio conferma
    {
        clientRemove(context, NULL, socket);
        return false;
    }
    else
        return true;
}

void clientRemove(ClientsContext *context, TopicsContext *topics, int socket)
{
    Client * client = context->clients[socket];

    sendCommand(socket, CMD_STOP); // Prova a inviare e ignora eventuali errori
    
    close(socket);

    if (client)
    {
        context->nClients--;
        if (client->registered)
            context->registered--;

        if (client->sending)
            context->sending--;

        if (client->game.questions)
            free(client->game.questions);
        
        if (client->game.score)
        {
            #ifdef KEEP_SCORE_ON_CLIENT_REMOVE

            if (client->game.playing != -1)
            {
                scoreboard_completedScore(&context->scoreboard, client->game.score[client->game.playing], client->game.playing);
                client_setPlayed(client, topics, client->game.playing, true);
            }
            
            #else

            for (int t = 0; t < context->scoreboard.nTopics; t++)
                if (client->game.score[t])
                {
                    if (t == client->game.playing)
                    {
                        client_setPlayed(client, topics, client->game.playing, true);
                        scoreboard_removeScore(&context->scoreboard, client->game.score[t], SCR_PLAYING, t);
                    }
                    else
                        scoreboard_removeScore(&context->scoreboard, client->game.score[t], SCR_COMPLETED, t);
                }
            
            #endif

            free(client->game.score);
        }

        if (client->game.playableTopics)
            free(client->game.playableTopics);

        if (client->operation)
        {
            list_destroyPreorder(client->operation, operationDestroy);
            client->operation = NULL;
        }

        #ifndef KEEP_SCORE_ON_CLIENT_REMOVE // I punteggi hanno bisogno del nome del client

            if (client->name)
                free(client->name);
        
        #endif

        free(client);

        // if (context->fd_max == socket)
        //     context->fd_max--;
    }

    context->clients[socket] = NULL;
    
    FD_CLR(socket, &context->master_fds);
    FD_CLR(socket, &context->write_fds);
    FD_CLR(socket, &context->read_fds);

    return;
}

void clientsFree(ClientsContext * context, TopicsContext *topics)
{
    if (!context)
        return;

    for (int i = 0; i <= context->fd_max; i++)
        if (isClient(context, i, false))
            clientRemove(context, topics, i);
    
    close(context->listener);

    free(context->clients);

    printf("Socket chiusi\n");

    return;
}

inline bool isClient(ClientsContext *context, int socket, bool onlyRegistered)
{
    if (FD_ISSET(socket, &context->master_fds))
    {
        if (!onlyRegistered)
            return true;
        else if (context->clients[socket]->registered)
            return true;
    }
    return false;
}

/* 
 * Si occupa di inviare dati grezzi, il funzionamento è il seguente:
 * Se block == false: invio dati normalmente, ritorno OP_DONE o OP_OK in base alla variabile errno
 * Se block == true: invio dati e (in base a errno) ciclo finché non ho terminato il messaggio
 */
OperationResult sendData(int socket, void *buffer, unsigned int length, unsigned int *sent, bool block)
{          
    OperationResult ret_block = OP_DONE;

    while (true)
    {
        if (block)
            if (!client_socketWriteReady(socket, &(struct timeval) {BLOCKING_SEND_TIMEOUT,  0})) // Salvo cicli CPU e ho un timeout per lo stallo del socket
                return OP_FAIL;

        int tmp = send(socket, buffer + *sent, length - *sent, 0);

        if (tmp >= 0)
        {
            *sent += tmp;
            if (*sent == length)
            {
                // Se bloccante ritorna OP_DONE in caso il socket non abbia bloccato, 
                // altrimenti OP_OK in maniera da proseguire con gli altri client
                if (!block)
                    return OP_DONE;
                else
                    return ret_block;
            }
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (!block)
                    return OP_OK;  // Se non bloccante, ritorna e ritenta più tardi (select)
                else
                    ret_block = OP_OK; // Se bloccante insisti fino al termine del messaggio corrente, e prosegui con altri clients
            }
            else 
                return OP_FAIL;  // Qualcosa è andato storto
        }
    }
}

Command recvCommand(int socket)
{
    int ret,
        sent = 0;

    u_int8_t tmp;

    while (sent < sizeof(tmp))
    {
        if ((ret = recv(socket, &tmp + sent, sizeof(tmp)- sent, 0)) > 0)
            sent += ret;
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;
        else // Non ho ricevuto niente
            return false;
    }

    return (enum Command) tmp;
}

Command nameValid(ClientsContext * context, int socket, char * name)
{
    if (!name)
        return CMD_STOP;

    if (strlen(name) > CLIENT_NAME_MAX || strlen(name) < CLIENT_NAME_MIN)
        return CMD_NOTVALID;

    for (char *c = name; *c; c++)
        if (!isalnum( (uint8_t) *c) && !isspace( (uint8_t) *c))
            return CMD_NOTVALID;

    for (int i = 0; i <= context->fd_max; i++)
        if (i != socket && isClient(context, i, true))
            if (context->clients[i]->name && !stricmp(name, context->clients[i]->name))
                return CMD_EXISTING;

    return CMD_OK;
}

bool sendCommand(int socket, enum Command cmd)
{
    int ret,
        received = 0;

    u_int8_t tmp = (u_int8_t) cmd;

    while (received < sizeof(tmp))
    {
        // Se il socket non è pronto per oltre COMMAND_SEND_TIMEOUT termino
        if (!client_socketWriteReady(socket, &(struct timeval) {COMMAND_SEND_TIMEOUT, 0}))
            return false;

        if ((ret = send(socket, &tmp + received, sizeof(tmp)- received, 0)) > 0)
            received += ret;
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;
        else
            return false;
    }

    if (cmd == CMD_STOP)
        return false; // Rimuovi già il client
    else
        return true;
}

bool client_gameInit(ClientsContext * context, int socket, TopicsContext * topicsContext)
{
    Client * client = context->clients[socket];

    client->game.playing = -1;
    client->game.currentQuestion = -1;
    client->game.questions = NULL;

    // Array topics giocabili
    client->game.playableTopics = malloc(sizeof(bool) * topicsContext->nTopics);
    if (!client->game.playableTopics)
        return false;
    memset(client->game.playableTopics, 0, sizeof(bool) * topicsContext->nTopics);
    
    // Array punteggi
    client->game.score = malloc(sizeof (DNode *) * topicsContext->nTopics);
    if (!client->game.score)
        return false;

    // Topics giocati e riempimento punteggi
    int * scores = topicsPlayed(topicsContext, client->name);

    for (int i = 0; i < topicsContext->nTopics; i++)
        if (scores[i] == -1)
        {
            client->game.playableTopics[i] = true;
            client->game.score[i] = NULL;
        }
        else // topic giocato
        {
            client->game.playableTopics[i] = false;
            
            #ifdef RELOAD_SCORES
            
                client->game.score[i] = scoreboard_get(&context->scoreboard, SCR_COMPLETED, i, client->name);
                if (!client->game.score[i])
                    return false;
                
                ((Score *) client->game.score[i]->data)->score = scores[i];
                listDoubly_sortElement(&context->scoreboard.scores[SCR_COMPLETED][i], NULL, client->game.score[i], scoreboard_scoreCompare);
            
            #endif
        }
    
    free(scores);

    return true;
}

bool client_quizInit(ClientsContext * context, int socket, TopicsContext *topicsContext)
{
    Client * client = context->clients[socket];
    Topic *currentTopic = &topicsContext->topics[client->game.playing];

    if (client->game.questions)
        free(client->game.questions);
    
    // Alloco l'array delle domande
    client->game.questions = malloc(sizeof(Question *) * currentTopic->nQuestions);
    if (!client->game.questions)
        return false;


    // Eseguo la copia di tutti i puntatori alle domande nello array delle domande
    int count = 0;
    for (Node * tmp = currentTopic->questions; tmp; tmp = tmp->next)
    {
        if (count >= currentTopic->nQuestions)
        {
            printf("Errore nel numero di domande\n"); // Andrebbe usato un logger
            break;
        }
        client->game.questions[count] = (Question *) tmp->data;
        count++;
    }

    Scoreboard * scoreboard = &context->scoreboard;

    client->game.score[client->game.playing] = scoreboard_get( scoreboard, SCR_PLAYING, client->game.playing, client->name);
    if (!client->game.score[client->game.playing])
        return false;

    shuffleArrayPtr((void*) client->game.questions, currentTopic->nQuestions);

    return true;
}   

int client_playableIndex(Client * client, TopicsContext * topics, int playable)
{
    if (!client || !topics || !client->game.playableTopics)
        return -1;

    if (playable < 0 || playable >= topics->nTopics)
        return -1;
    
    for (int i = 0, n = 0; i < topics->nTopics; i++)
        if (client->game.playableTopics[i])
            if (n++ == playable)
                    return i;

    return -1;
}

bool client_checkTopicIndex(Client * client, TopicsContext * topics, int playable)
{
    if (!client || !topics || !client->game.playableTopics)
        return false;

    if (playable < 0 || playable >= topics->nTopics)
        return false;
    
    if (client->game.playableTopics[client->game.playing])
        return true;

    return false;
}

bool client_setPlayed(Client * client, TopicsContext * topics, int topic, bool writeScore)
{
    if (topic < 0 || topic >= topics->nTopics)
        return false;
    
    if (client->game.playableTopics[topic]) // Il topic è già stato giocato?
    {
        client->game.playableTopics[topic] = false;

        if (!topicMakePlayed(topics, client->name, topic, -1))
            return false;
    }
    else if (!writeScore) // Se non sono qui per scrivere il punteggio c'è stato un errore
        return false;
    
    if (writeScore)
    {
        if (!(client->game.playing == topic && client->game.score[topic])) // Non sto salvando il topic che sto giocando
            return false;
        
        int score = ((Score*) client->game.score[topic]->data)->score;

        if (!topicMakePlayed(topics, client->name, topic, score))
            return false;
    }
    
    return true;
}

OperationResult client_sendTopics(ClientsContext *context, int socket, TopicsContext *topics)
{
    Client * client = context->clients[socket];

    if (!sendCommand(socket, CMD_OK))
        return false;

    MessageArray * tmp = MessageArrayCpy(topics->topicsString);

    if (isClient(context, socket, true))
        messageBoolArray( &((MessageArray *) tmp)->messages[tmp->size - 1],
                            client->game.playableTopics,
                            topics->nTopics);
    else
        memset(&tmp->messages[tmp->size - 1], 0, sizeof(Message)); // Se non fosse registrato non mando niente
    
    return operationCreate(sendMessage, context, socket, tmp);
}

OperationResult recvData(int socket, void *buffer, unsigned int lenght, unsigned int *received)
{
    int ret;

    ret = recv(socket, buffer + *received, lenght - *received, 0);

    if (ret > 0)
        *received += ret;
    else if (ret == 0)
        return OP_FAIL;
    else // ret < 0
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) // Ripeti l'operazione
            return OP_OK;
        else
            return OP_FAIL;
    }

    if (lenght == *received)
        return OP_DONE;
    else
        return OP_OK; // Non è detto di ricevere tutti i byte voluti
}

bool client_socketReady(int socket, struct timeval * timeout)
{
    fd_set test_fds;
    FD_ZERO(&test_fds);
    FD_SET(socket, &test_fds);
    if (select(socket + 1, &test_fds, NULL, NULL, timeout ) > 0)
        return true;
    return false;
}

bool client_socketWriteReady(int socket, struct timeval * timeout)
{
    fd_set test_fds;
    FD_ZERO(&test_fds);
    FD_SET(socket, &test_fds);
    if (select(socket + 1, NULL, &test_fds, NULL, timeout ) > 0)
        return true;
    return false;
}

OperationResult client_sendScoreboard(ClientsContext *context, int socket)
{
    if (!sendCommand(socket, CMD_OK))
        return false;

    MessageArray * tmp = messageArray(2 * context->scoreboard.nTopics);
    if (!tmp)
        return false;

    // La serializzazione della scoreboard non può essere aggiornata se non tutti i dati
    // della singola serializzazione non sono stati inviati nel caso il socket diventasse
    // non ready
    tmp->isInterruptible = false;

    for (int i = 0; i < SCOREBOARD_SIZE * context->scoreboard.nTopics; i++)
        messageStringReady(&tmp->messages[i], 
                            context->scoreboard.serialized[i].string, 
                            context->scoreboard.serialized[i].serialized_lenght,
                            false);

    return operationCreate(sendMessage, context, socket, tmp);
}

OperationResult client_sendPlayable(ClientsContext *context, TopicsContext * topics, int socket)
{
    Client * client = context->clients[socket];

    if (!sendCommand(socket, CMD_OK))
        return false;

    MessageArray *tmp = messageArray(1);
    if (!tmp)
        return false;

    messageBoolArray( &((MessageArray *) tmp)->messages[0], client->game.playableTopics, topics->nTopics);
    return operationCreate(sendMessage, context, socket, tmp);
}

void client_endquiz(ClientsContext *context, int socket, TopicsContext *topics)
{
    Client * client = context->clients[socket];

    scoreboard_completedScore(&context->scoreboard, client->game.score[client->game.playing],
                    client->game.playing);

    client_setPlayed(client, topics, client->game.playing, true);

    client->game.playing = -1;
    client->game.currentQuestion = -1;
}