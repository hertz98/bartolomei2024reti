#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "clients.h"

int clientsInit(ClientsContext **context, int max)
{
    if (max < 0 || max > 1024)
        return 1;

    *context = (ClientsContext *) malloc( sizeof(ClientsContext) );

    if (!context)
        return 1;

    memset(*context, 0, sizeof(ClientsContext));

    (*context)->nClients = 0;
    (*context)->maxClients = max;
    (*context)->clients = malloc( sizeof(Client *) * max);
    (*context)->allocated = max;
    FD_ZERO(&(*context)->master);

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
        context->allocated += increment;
    }

    Client ** client = &context->clients + socket;
    *client = (Client *) malloc(sizeof(Client));
    if (! client)
        return false;

    context->nClients++;
    context->fd_max = socket;
    memset(*client, 0, sizeof(Client));

    (*client)->socket = socket;
    (*client)->registered = false;
    (*client)->operation = NULL;
    (*client)->step = 0;
    (*client)->recv_timestamp = time(NULL);

    FD_SET(socket, &context->master);

    return true;
}

void clientRemove(ClientsContext * context, int socket)
{
    Client ** client = &context->clients +socket;

    sendCommand(*client, CMD_STOP); // Prova a inviare e ignora eventuali errori
    close((*client)->socket);

    if (FD_ISSET(socket, &context->master))
    {
        if((*client)->registered)
            free((*client)->name);

        free(*client);

        context->nClients--;
        // if (context->fd_max == socket)
        //     context->fd_max--;
    }
    
    FD_CLR(socket, &context->master);

    return;
}

void clientsFree(ClientsContext * context, int socket)
{
    for (int i = 0; i < context->fd_max; i++)
        if (FD_ISSET(i, &context->master))
            clientRemove(context, i);
    
    free(context->clients);
    free(context);

    return;
}

OperationStatus sendMessage(Client * client, void * buffer, bool init)
{
    OperationStatus ret = OP_FAIL;

    if (init)
    {
        client->step = 0;
        client->operation = sendMessage;
        client->tmp_p = buffer;
    }

    switch (client->step++)
    {
        case 0:
            if (sendCommand(client, CMD_MESSAGE))
                ret = OP_OK;
            break;

        case 1:
            if (recvCommand(client) == CMD_SIZE &&
             sendInteger(client, strlen(client->tmp_p)))
                ret = OP_OK;
            break;

        case 2:
            if (recvCommand(client) == CMD_STRING &&
             sendString(client, client->tmp_p, strlen(client->tmp_p)))
                ret = OP_OK;
            break;

        case 3:
            if (recvCommand(client) == CMD_OK)
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
            client->operation = NULL;
            break;
        default:
            return OP_FAIL;
    }

    return ret;
}

bool sendInteger(Client * client, int i)
{
    int ret;

    u_int32_t tmp = htonl(i);

    if ((ret = send(client->socket, &tmp, sizeof(tmp), 0)) != sizeof(tmp))
    {
        if (errno)
            perror("sendInteger failed");
        return false;    
    }
    
    return true;
}

bool sendString(Client * client, char * buffer, int lenght)
{
    int ret;

    if ((ret = send(client->socket, buffer, lenght, 0)) != lenght)
        return false;

    return true;
}

enum Command recvCommand(Client *client)
{
    int ret;

    u_int8_t tmp;

    if ((ret = recv(client->socket, &tmp, sizeof(tmp), 0) != sizeof(tmp)))
    {
        if (errno)
            perror("recvCommand failed");
        return false;    
    }

    client->recv_timestamp = time(NULL);

    return (enum Command) tmp;
}

OperationStatus recvMessage(Client * client, void * buffer, bool init)
{
    OperationStatus ret = OP_FAIL;

    if (init)
    {
        client->tmp_p = buffer;
        client->step = 0;
        client->operation = recvMessage;
    }

    switch (client->step++)
    {
        case 0: // Il client non invia mai una stringa senza preavviso
            if (recvCommand(client) == CMD_MESSAGE &&
             sendCommand(client, CMD_SIZE))
                ret = OP_OK;
            break;

        case 1:
            if ((client->tmp_i = recvInteger(client)) > 0 &&
             sendCommand(client, CMD_STRING))
                ret = OP_OK;
            break;

        case 2:
            if (recvString(client, (char **) client->tmp_p, client->tmp_i) &&
             sendCommand(client, CMD_OK))
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
            client->operation = NULL;
            break;
        default:
            return OP_FAIL;
    }

    return ret;
}

bool recvString(Client * client, char ** buffer, int lenght)
{
    int ret;

    *buffer = (char *) malloc(sizeof(char) * lenght);
    if (!buffer)
        return false;

    if ((ret = recv(client->socket, *buffer, lenght, 0)) != lenght)
    {
        free(*buffer);
        *buffer = NULL;
        if (errno)
            perror("recvString");
        return false;
    }

    client->recv_timestamp = time(NULL);

    return true;
}

OperationStatus regPlayer(Client * client, void * p, bool init)
{

    Client **clients = p;

    if (init)
    {
        client->tmp_p = &client->name;
        client->step = 0;
        client->operation = regPlayer;
    }
    
    OperationStatus ret;
    switch( ret = recvMessage(client, NULL, false) )
    {
        case OP_OK:
            break;
        case OP_FAIL:
            client->operation = NULL;
            break;

        case OP_DONE:
            client->operation = NULL;
            if (nameValid(clients, client->name))
                {
                    client->registered = true;
                    printf("%s\n",client->name); // DEBUG
                    sendCommand(client, CMD_OK);
                    sendMessage(client, "provasendfromserver", true);
                    return true;
                }
                else
                {
                    sendCommand(client, CMD_STOP);
                    return false;
                }
            break;

        default:
            return OP_FAIL;
    }
    return (bool) ret;
}

bool nameValid(Client ** clients, char * name)
{
    // To Do
    return true;
}

int recvInteger(Client * client)
{
    int ret;

    u_int32_t tmp;

    if ((ret = recv(client->socket, &tmp, sizeof(tmp), 0) != sizeof(tmp)))
    {
        if (errno)
            perror("recvInteger failed");
        return false;    
    }

    client->recv_timestamp = time(NULL);

    return ntohl(tmp);
}

bool sendCommand(Client * client, enum Command cmd)
{
    int ret;

    u_int8_t tmp = (u_int8_t) cmd;

    if ((ret = send(client->socket, &tmp, sizeof(tmp), 0)) != sizeof(tmp))
    {
        if (errno)
            perror("sendCommand failed");
        return false;    
    }
    
    return true;
}

