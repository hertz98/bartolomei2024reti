#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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

    memset(clients, 0, sizeof(struct Client));

    client->socket = socket;
    client->name[0] = '\0';
    client->operation = NULL;
    client->step = 0;
    client->recv_timestamp = time(NULL);

    FD_SET(socket, master);

    return true;
}

void clientRemove(fd_set * master, struct Client ** clients, int socket)
{
    struct Client * client = clients[socket];

    if (FD_ISSET(socket, master))
        free(client);
    
    FD_CLR(socket, master);

    return;
}

void clientFree(fd_set * master, struct Client ** clients, int max_clients)
{
    for (int i = 0; i < max_clients; i++)
        if (FD_ISSET(i, master))
        {
            sendCommand(clients[i], CMD_STOP);
            free(clients + i);
        }
    
    return;
}

bool sendMessage(struct Client * client, void * buffer)
{
    if (buffer != NULL){
        client->step = 0;
        client->operation = sendMessage;
        client->tmp_p = buffer;
    }
    
    // Disfaccio tutto
    if (!sendMessageProcedure(client))
    {
        client->step = 0;
        client->operation = NULL;
        return false;
    }
    
    return true;
}

bool sendMessageProcedure(struct Client * client)
{
    client->step++;

    switch (client->step)
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
            return sendString(client->socket, client->tmp_p, strlen(client->tmp_p));

        case 3:
            if (recvCommand(client) != CMD_OK)
                return false;
            client->operation = NULL;
            client->step = 0;

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
        perror("sendCommand failed");
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

    return (enum Command) ntohl(tmp);
}

bool recvMessage(struct Client * client, void * buffer)
{
    if (buffer != NULL){
        client->step = 0;
        client->operation = recvMessage;
    }
    
    // Disfaccio tutto
    if (!sendMessage(client, buffer))
    {
        client->step = 0;
        client->operation = NULL;
        return false;
    }
    
    return true;
}

bool recvMessageProcedure(struct Client * client)
{
    client->step++;

    switch (client->step)
    {
        case 0:
            return sendCommand(client, CMD_SIZE);
            break;

        case 1:
            client->tmp_i = recvInteger(client);
            return sendCommand(client, CMD_STRING);
            break;

        case 2:
            if (!(recvString(client->socket, client->tmp_p, client->tmp_i)))
            {
                free(client->tmp_p);
                return false;
            }
            
            client->operation = NULL;
            client->step = 0;

            return sendCommand(client, CMD_OK);
            break;

        default:
            return false;
    }
}

bool recvString(int socket, char * buffer, int lenght)
{
    int ret;

    buffer = (char *) malloc(sizeof(char) * lenght);

    if ((ret = recv(socket, buffer, lenght, 0)) != lenght)
    {
        free(buffer);
        return false;
    }

    return true;
}

int recvInteger(struct Client * client)
{
    int ret;

    u_int32_t tmp;

    if ((ret = recv(client->socket, &tmp, sizeof(tmp), 0) != sizeof(tmp)))
    {
        perror("recvCommand failed");
        return false;    
    }

    client->recv_timestamp = time(NULL);

    return ntohl(tmp);
}

bool sendCommand(struct Client * client, enum Command cmd)
{
    int ret;

    u_int8_t cmd_n = (u_int8_t) cmd;

    if ((ret = send(client->socket, &cmd_n, sizeof(cmd_n), 0)) != sizeof(cmd_n))
    {
        perror("sendCommand failed");
        return false;    
    }
    
    return true;
}

