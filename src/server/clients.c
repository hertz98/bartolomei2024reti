#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "clients.h"

bool clientAdd(fd_set * master, struct Client ** clients, int socket)
{
    struct Client * client = (struct Client *) malloc(sizeof(struct Client));

    if (! client)
        return false;

    clients[socket] = client;

    memset(client, 0, sizeof(struct Client));

    client->socket = socket;
    client->registered = false;
    client->operation = NULL;
    client->step = 0;
    client->recv_timestamp = time(NULL);

    FD_SET(socket, master);

    return true;
}

void clientRemove(fd_set * master, struct Client ** clients, int socket)
{
    struct Client * client = clients[socket];

    sendCommand(client, CMD_STOP); // Prova a inviare e ignora eventuali errori
    close(client->socket);

    if (FD_ISSET(socket, master))
    {
        if(client->registered)
            free(client->name);
        free(client);
    }
    
    FD_CLR(socket, master);

    return;
}

void clientFree(fd_set * master, struct Client ** clients, int max_clients)
{
    for (int i = 0; i < max_clients; i++)
        if (FD_ISSET(i, master))
            clientRemove(master, clients, i);
    
    return;
}

enum OperationStatus sendMessage(struct Client * client, void * buffer, bool init)
{
    if (init)
    {
        client->step = -1;
        client->operation = sendMessage;
        client->tmp_p = buffer;
    }
    
    enum OperationStatus ret = sendMessageProcedure(client);

    if (ret == OP_DONE || ret == OP_FAIL)
    {
            client->step = 0;
            client->operation = NULL;
            return ret;
    }

    return true;
}

enum OperationStatus sendMessageProcedure(struct Client * client)
{
    switch (++client->step)
    {
        case 0:
            return sendCommand(client, CMD_MESSAGE);
            break;

        case 1:
            if (recvCommand(client) != CMD_SIZE)
                return false;
            return sendInteger(client, strlen(client->tmp_p));
            break;

        case 2:
            if (recvCommand(client) != CMD_STRING)
                return false;
            return sendString(client, client->tmp_p, strlen(client->tmp_p));

        case 3:
            if (recvCommand(client) != CMD_OK)
                return false;
            return OP_DONE;
            
        default:
            return false;
    }
    return true;
}

bool sendInteger(struct Client * client, int i)
{
    int ret;

    u_int32_t tmp = htonl(i);

    if ((ret = send(client->socket, &tmp, sizeof(tmp), 0)) != sizeof(tmp))
    {
        perror("sendInteger failed");
        return false;    
    }
    
    return true;
}

bool sendString(struct Client * client, char * buffer, int lenght)
{
    int ret;

    if ((ret = send(client->socket, buffer, lenght, 0)) != lenght)
        return false;

    return true;
}

enum Command recvCommand(struct Client *client)
{
    int ret;

    u_int8_t tmp;

    if ((ret = recv(client->socket, &tmp, sizeof(tmp), 0) != sizeof(tmp)))
    {
        perror("recvCommand failed");
        return false;    
    }

    client->recv_timestamp = time(NULL);

    return (enum Command) tmp;
}

enum OperationStatus recvMessage(struct Client * client, void * buffer, bool init)
{
    enum OperationStatus ret = OP_FAIL;

    if (init)
    {
        client->tmp_p = buffer;
        client->step = -1;
        client->operation = recvMessage;
    }

    switch (++client->step)
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

bool recvString(struct Client * client, char ** buffer, int lenght)
{
    int ret;

    *buffer = (char *) malloc(sizeof(char) * lenght);
    if (!buffer)
        return false;

    if ((ret = recv(client->socket, *buffer, lenght, 0)) != lenght)
    {
        free(*buffer);
        *buffer = NULL;
        return false;
    }

    client->recv_timestamp = time(NULL);

    return true;
}

enum OperationStatus regPlayer(struct Client * client, void * p, bool init)
{

    struct Client **clients = p;

    if (init)
    {
        client->tmp_p = &client->name;
        client->step = -1;
        client->operation = regPlayer;
    }
    
    enum OperationStatus ret;
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

bool nameValid(struct Client ** clients, char * name)
{
    // To Do
    return true;
}

int recvInteger(struct Client * client)
{
    int ret;

    u_int32_t tmp;

    if ((ret = recv(client->socket, &tmp, sizeof(tmp), 0) != sizeof(tmp)))
    {
        perror("recvInteger failed");
        return false;    
    }

    client->recv_timestamp = time(NULL);

    return ntohl(tmp);
}

bool sendCommand(struct Client * client, enum Command cmd)
{
    int ret;

    u_int8_t tmp = (u_int8_t) cmd;

    if ((ret = send(client->socket, &tmp, sizeof(tmp), 0)) != sizeof(tmp))
    {
        perror("sendCommand failed");
        return false;    
    }
    
    return true;
}

