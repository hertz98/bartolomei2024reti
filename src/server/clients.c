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
        if (client->name)
            free(client->name);
        if (client->game.playableTopics)
            free(client->game.playableTopics);
        if (client->game.questions)
            free(client->game.questions);
        if (client->game.score)
            free(client->game.questions);

        free(client);

        context->nClients--;
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

        if (msg->data)
        {
            unsigned int sent = msg->transmitted - lenght_size;

            if ((ret = sendData(socket, msg->data, msg->lenght, &sent)) != OP_DONE)
                return ret;
        }

        client->transferring++;
    }

    client->transferring = false;
    // TODO: Dealloco client->toSend
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
    int ret;

    u_int8_t tmp;

    if ((ret = recv(socket, &tmp, sizeof(tmp), 0) != sizeof(tmp)))
    {
        if (!ret) // Client disconnesso
            return false;

        if (errno)
            perror("recvCommand failed");
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
    int ret;

    u_int8_t tmp = (u_int8_t) cmd;

    if ((ret = send(socket, &tmp, sizeof(tmp), 0)) != sizeof(tmp))
    {
        if (errno)
            perror("sendCommand failed");
        return false;    
    }
    
    return true;
}

bool gameInit(Client * client, TopicsContext * topicsContext)
{
    client->game.playing = -1;
    client->game.currentQuestion = -1;
    client->game.playableTopics = topicsUnplayed(topicsContext, client->name);
    client->game.questions = NULL;

    client->game.score = malloc(sizeof (int) * topicsContext->nTopics);
    if (!client->game.score)
        return false;
    for (int i = 0; i < topicsContext->nTopics; i++)
        client->game.score[i] = -1;

    return true;
}

OperationResult regPlayer(ClientsContext *context, int socket, void *topicsContext)
{
    
}

Operation *getOperation(Client * client, void * function)
{
    for(int i = 0; i < MAX_STACKABLE_OPERATIONS; i++)
    {
        if (client->operation[i].function == function || client->operation[i].function == NULL)
            return &client->operation[i];
    }
    return NULL;
}

OperationResult sendMessage(ClientsContext *context, int socket, void *message_array)
{
    Client *client = context->clients[socket];

    Operation *currentOperation = getOperation(client, sendMessage);
    if (!currentOperation)
        return OP_FAIL;

    if (!currentOperation->function)
    {
        currentOperation->function = sendMessage;
        currentOperation->step = 0;
        currentOperation->p = message_array;
    }

    MessageArray *msgs = (MessageArray *) message_array;
    OperationResult ret = OP_FAIL;

    switch (currentOperation->step++)
    {
        case 0:
            if (sendCommand(socket, CMD_MESSAGE))
            {
                client->toSend = msgs;
                client->transferring = -1;
                FD_SET(socket, &context->write_fds);   
                ret = OP_OK;
            }
            break;
        case 1:
            if (recvCommand(socket) == CMD_OK)
                ret = OP_DONE;
            break;
    }

    if (ret == OP_DONE)
        memset(&client->operation, 0, sizeof(client->operation));

    return ret;
}

OperationResult recvMessage(ClientsContext *context, int socket, void *pointer)
{
    Client * client = context->clients[socket];
    OperationResult ret = OP_OK;

    Operation *currentOperation = getOperation(client, recvMessage);
    if (!currentOperation)
        return OP_FAIL;

    MessageArray ** msgs = (MessageArray **) pointer;

    if (!currentOperation->function)
    {
        currentOperation->function = recvMessage;
        currentOperation->p = pointer;
        *msgs = messageArray(0);
        client->transferring = -1;
    }

    if (client->transferring == -1)
    {
        Message *tmp = &(*msgs)->messages[0];
        if ((ret = recvData(socket, &tmp->lenght, sizeof(tmp->lenght), &tmp->transmitted)) == OP_DONE)
        {
            (*msgs)->size = ntohl(tmp->lenght);
            (*msgs)->messages = realloc((*msgs)->messages, sizeof(Message) * (*msgs)->size);
            memset((*msgs)->messages, 0, sizeof(Message) * (*msgs)->size);
            client->transferring = 0;
        }
        return (bool) ret;
    }
    
    while (client->transferring < (*msgs)->size)
    {
        Message * msg = &(*msgs)->messages[client->transferring];

        if (msg->transmitted < sizeof(msg->lenght))
        {
            if ((ret = recvData(socket, &msg->lenght, sizeof(msg->lenght), &msg->transmitted)) == OP_DONE)
                msg->lenght = ntohl(msg->lenght);
            else
                return (bool) ret;
        }

        if (msg->lenght)
        {
            msg->data = (void *) malloc(msg->lenght);

            unsigned int sent = msg->transmitted - sizeof(msg->lenght);

            if ((ret = recvData(socket, msg->data, msg->lenght, &sent)) != OP_DONE)
                return (bool) ret;

        }

        client->transferring++;
    }

    client->transferring = false;
    memset(&client->operation, 0, sizeof(client->operation));

    return sendCommand(socket, CMD_OK) ? OP_DONE : OP_FAIL;
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