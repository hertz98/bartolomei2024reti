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
#include "../shared/message.h"

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
    client->currentOperation = NULL;
    client->step = 0;
    client->toSend = NULL;
    client->sending = false;
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
    
    if (!client->sending)
    {
        list_append(&client->toSend, emptyMessage());
        client->sending = true;
    }

    OperationResult ret;
    for (Node * node = client->toSend; node; node = node->next)
    {
        Message * msg = node->data;

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

        list_extractHead(&client->toSend); // TODO: distruggi l'elemento appena estratto
    }

    client->sending = false;
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

OperationResult legacysendMessage(ClientsContext *context, int socket, void * message, bool init)
{
    OperationResult ret = OP_FAIL;

    Client * client = context->clients[socket];

    if (init)
    {
        client->step = 0;
        client->currentOperation = legacysendMessage;
    }

    switch (client->step++)
    {
        case 0:
            if (sendCommand(socket, CMD_MESSAGE) &&
                list_append(&client->toSend, message) // Il messaggio vuoto indica la fine della 
                )
            {
                FD_SET(socket, &context->write_fds);
                ret = OP_OK;
            }
            break;

        case 1:
            if (recvCommand(socket) == CMD_OK)
                ret = OP_DONE;
            break;
            
        default:
            return false;
    }

    switch(ret)
    {
        case OP_OK:
            break;
        case OP_FAIL:
        case OP_DONE:
            client->currentOperation = NULL;
            break;
        default:
            return OP_FAIL;
    }

    return ret;
}

bool sendInteger(int socket, int i)
{
    int ret;

    u_int32_t tmp = htonl(i);

    if ((ret = send(socket, &tmp, sizeof(tmp), 0)) != sizeof(tmp))
    {
        if (errno)
            perror("sendInteger failed");
        return false;    
    }
    
    return true;
}

bool sendString(int socket, char * buffer, int lenght)
{
    int ret;

    if ((ret = send(socket, buffer, lenght, 0)) != lenght)
        return false;

    return true;
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

OperationResult legacyrecvMessage(ClientsContext *context, int socket, void * buffer, bool init)
{
    OperationResult ret = OP_FAIL;

    Client * client = context->clients[socket];

    if (init)
    {
        client->tmp_p = buffer;
        client->step = 0;
        client->currentOperation = legacyrecvMessage;
    }

    switch (client->step++)
    {
        case 0: // Il client non invia mai una stringa senza preavviso
            if (recvCommand(socket) == CMD_MESSAGE &&
             sendCommand(socket, CMD_RECVMESSAGE))
                ret = OP_OK;
            break;

        case 1:
            if ((client->tmp_i = recvInteger(socket)) > 0 &&
             recvString(socket, (char **) client->tmp_p, client->tmp_i) &&
             sendCommand(socket, CMD_OK))
                ret = OP_DONE;
            break;

        default:
            return false;
    }

    switch(ret)
    {
        case OP_OK:
            break;
        case OP_FAIL:
        case OP_DONE:
            client->currentOperation = NULL;
            break;
        default:
            return OP_FAIL;
    }

    return ret;
}

bool recvString(int socket, char ** buffer, int lenght)
{
    int ret;

    *buffer = (char *) malloc(sizeof(char) * (lenght + 1));
    if (!buffer)
        return false;

    if ((ret = recv(socket, *buffer, lenght, 0)) != lenght)
    {
        free(*buffer);
        *buffer = NULL;
        
        if (!ret) // Client disconnesso
            return false;

        if (errno)
            perror("recvString");
        return false;
    }

    (*buffer)[ret] = '\0';
    (*buffer)[lenght] = '\0';

    return true;
}

OperationResult regPlayer(ClientsContext *context, int socket, void * p, bool init)
{
    Client * client = context->clients[socket];
    TopicsContext * topicsContext = p;

    if (init)
    {
        client->tmp_p = &client->name;
        client->step = 0;
        client->currentOperation = regPlayer;
        client->tmp_p2 = p;
    }
    
    OperationResult ret;
    switch( ret = legacyrecvMessage(context, socket, NULL, false) )
    {
        case OP_OK:
            break;
        case OP_FAIL:
            client->currentOperation = NULL;
            break;

        case OP_DONE:
            client->currentOperation = NULL;
            if (nameValid(context, socket, client->name))
                {
                    printf("%s\n",client->name); // DEBUG
                    if (sendCommand(socket, CMD_OK) && gameInit(context->clients[socket], topicsContext))
                    {
                        client->registered = true;   
                        return OP_DONE;
                    }
                    else
                        return false;
                }
                else
                {
                    free(client->name);
                    client->name = NULL;
                    sendCommand(socket, CMD_NOTVALID);
                    return true;
                }
            break;

        default:
            return OP_FAIL;
    }
    return (bool) ret;
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

int recvInteger(int socket)
{
    int ret;

    u_int32_t tmp;

    if ((ret = recv(socket, &tmp, sizeof(tmp), 0) != sizeof(tmp)))
    {
        if (!ret) // Client disconnesso
            return false;
        
        if (errno)
            perror("recvInteger failed");
        return false;    
    }

    return ntohl(tmp);
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

OperationResult confirmedOperation(ClientsContext *context, int socket, void * p, OperationResult (*operation)(ClientsContext *context, int socket, void *buffer))
{
    OperationResult ret = OP_FAIL;

    Client * client = context->clients[socket];

    if (operation) // Se è definita una funzione inizializza
    {
        client->operation.operationHandler = confirmedOperation;
        client->operation.operation = operation;
        client->operation.step = 0;
        client->operation.tmp = p;
    }

    switch (client->operation.step++)
    {
        case 0:
            if (operation(context, socket, p))
                ret = OP_OK;
            break;

        case 1:
            if (recvCommand(socket) == CMD_OK)
                ret = OP_DONE;
            break;
            
        default:
            return false;
    }

    switch(ret)
    {
        case OP_OK:
            break;
        case OP_FAIL:
        case OP_DONE:
            memset(&client->operation, 0, sizeof(client->operation));
            break;
        default:
            return OP_FAIL;
    }

    return ret;
}

OperationResult sendMessage(ClientsContext *context, int socket, void *message)
{
    Client *client = context->clients[socket];
    Message *msg = (Message *) message;

    OperationResult ret = OP_FAIL;

    if (sendCommand(socket, CMD_MESSAGE) &&
        list_append(&client->toSend, msg)
        )
    {
        FD_SET(socket, &context->write_fds);   
        ret = OP_DONE;
    }

    return ret;
}
