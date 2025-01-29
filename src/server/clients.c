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
#include "clients.h"
#include "util.h"

int clientsInit(ClientsContext *context, int max)
{
    if (max < 0 || max > 1024)
        return 1;

    memset(context, 0, sizeof(ClientsContext));

    context->nClients = 0;
    context->maxClients = max;
    context->clients = malloc( sizeof(Client *) * max);
    context->allocated = max;
    memset(context->clients, 0, context->allocated);
    FD_ZERO(&context->master_fds);
    FD_ZERO(&context->read_fds);
    FD_ZERO(&context->write_fds);

    return 0;
}

bool clientAdd(ClientsContext * context, int socket)
{
    const int increment = 10;

    if (context->nClients >= context->maxClients)
        return false;

    // Il numero di socket è usato per indirizzare le strutture dati
    // ma non necessariamente coincide col numero di clients
    if (socket > context->allocated){
        context->clients = realloc(context->clients, sizeof(Client *) * (socket + increment));
        memset(context->clients + context->allocated, 0, increment);
        context->allocated += increment;
    }

    context->clients[socket] = (Client *) malloc(sizeof(Client));
    Client * client = context->clients[socket];
    if (!client)
        return false;
    memset(client, 0, sizeof(Client));

    context->nClients++;
    if (socket > context->fd_max)
        context->fd_max = socket;

    client->name = NULL;
    client->registered = false;
    client->toSend = NULL;
    client->transferring = false;
    client->game.playableTopics = NULL;
    client->game.questions = NULL;
    client->game.score = NULL;
    client->operation = NULL;
    client->nOperations = 0;

    FD_SET(socket, &context->master_fds);

    int keepalive = 1;
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

    return true;
}

void clientRemove(ClientsContext * context, int socket)
{
    Client * client = context->clients[socket];

    sendCommand(socket, CMD_STOP); // Prova a inviare e ignora eventuali errori
    
    close(socket);

    if (client)
    {
        context->nClients--;
        if (client->registered)
            context->registered--;

        if (client->name)
            free(client->name);
        if (client->game.playableTopics)
            free(client->game.playableTopics);
        if (client->game.questions)
            free(client->game.questions);
        if (client->game.score)
            free(client->game.score);

        if (client->operation)
        {
            list_destroy(client->operation, operationDestroy);
            client->operation = NULL;
        }

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

void clientsFree(ClientsContext * context)
{
    if (!context)
        return;

    for (int i = 0; i <= context->fd_max; i++)
        if (isClient(context, i, false))
            clientRemove(context, i);
    
    close(context->listener);

    free(context->clients);

    printf("Socket chiusi\n");

    return;
}

inline bool isClient(ClientsContext *context, int socket, bool onlyRegistered)
{
    if (FD_ISSET(socket, &context->master_fds))
        if (!onlyRegistered || context->clients[socket]->registered)
            return true;
    return false;
}

OperationResult sendMessageHandler(ClientsContext *context, int socket)
{
    Client * client = context->clients[socket];

    if (!client->toSend)
        return false;
    
    OperationResult ret;
    while (client->transferring < client->toSend->size)
    {
        Message * msg;
        if (client->transferring == -1)
            msg = &client->toSend->messages[client->toSend->size];
        else
            msg = &client->toSend->messages[client->transferring];

        int lenght = htonl(msg->lenght);
        int lenght_size = sizeof(msg->lenght);

        if (msg->transmitted < lenght_size)
            if ((ret = sendData(socket, &lenght, lenght_size, &msg->transmitted)) != OP_DONE)
                return ret;

        if (msg->payload)
        {
            unsigned int sent = msg->transmitted - lenght_size;

            if ((ret = sendData(socket, msg->payload, msg->lenght, &sent)) != OP_DONE)
                return ret;
        }

        client->transferring++;
    }

    client->transferring = false;
    client->toSend = NULL;
    return OP_DONE;
}

OperationResult sendData(int socket, void *buffer, unsigned int lenght, unsigned int *sent)
{          
    int ret;

    ret = send(socket, buffer + *sent, lenght - *sent, 0);

    if (ret >= 0)
        *sent += ret;
    else
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) // Ripeti l'operazione, socket non pronto
            return OP_OK; 
        else 
            return OP_FAIL;
    }
    
    if (lenght == *sent)
        return OP_DONE;
    else
        return OP_OK;
}

enum Command recvCommand(int socket)
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
        else
            return false;
    }

    return (enum Command) tmp;
}

bool nameValid(ClientsContext * context, int socket, char * name)
{
    // Il nome non può essere più lungo di PATH_MAX - ESTENSIONE
    if (strlen(name) > NAME_MAX - strlen(".txt") && strlen(name) < 4)
        return false;

    for (int i = 0; i <= context->fd_max; i++)
        if (i != socket && isClient(context, i, false))
            if (strcmp(name, context->clients[i]->name) == 0)
                return false;
    
    if (!isAlphaNumeric(name))
        return false;

    return true;
}

bool sendCommand(int socket, enum Command cmd)
{
    int ret,
        received = 0;

    u_int8_t tmp = (u_int8_t) cmd;

    while (received < sizeof(tmp))
    {
        if ((ret = send(socket, &tmp + received, sizeof(tmp)- received, 0)) > 0)
            received += ret;
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;
        else
            return false;
    }

    return true;
}

bool client_gameInit(Client * client, TopicsContext * topicsContext)
{
    client->game.playing = -1;
    client->game.currentQuestion = -1;
    client->game.playableTopics = topicsUnplayed(topicsContext, client->name);
    client->game.nPlayable = 0;
    for (int i = 0; i < topicsContext->nTopics; i++)
        if (client->game.playableTopics[i])
            client->game.nPlayable++;

    client->game.questions = NULL;

    client->game.score = malloc(sizeof(int) * topicsContext->nTopics);
    if (!client->game.score)
        return false;
    for (int i = 0; i < topicsContext->nTopics; i++)
        client->game.score[i] = -1;

    return true;
}

bool client_quizInit(Client *client, TopicsContext *topicsContext)
{
    Topic *currentTopic = &topicsContext->topics[client->game.playing];

    if (client->game.questions)
        free(client->game.questions);
    
    client->game.questions = malloc(sizeof(Question *) * currentTopic->nQuestions);
    if (!client->game.questions)
        return false;

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

    // TODO: SHUFFLE

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

bool client_setPlayed(Client * client, TopicsContext * topics, int topic)
{
    if (topic < 0 || topic >= topics->nTopics)
        return false;

    if (!client->game.playableTopics[topic])
        return false;

    client->game.playableTopics[topic] = false;
    client->game.score[topic] = 0;
    if (client->game.nPlayable)
        client->game.nPlayable--;

    if (topicPlayed(topics, client->name, topic))
        return true;
    return false;
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
        return OP_OK;
}
