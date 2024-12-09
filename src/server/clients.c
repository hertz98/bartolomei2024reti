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

    FD_CLR(socket, master);

    if (FD_ISSET(socket, master))
        free(client);
    
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
    if (buffer != NULL)
        client->step = 0;
    
    switch (client->step)
    {
        case 0:
            client->operation = sendMessage;
            client->tmp = buffer;
            client->step++;
            return sendCommand(client, CMD_MESSAGE);
            break;

        case 1:
            client->step++;
            if (recvCommand(client) != CMD_OK)
                return false;
            return sendInteger(client, strlen(client->tmp));
            break;

        case 2:
            if (recvCommand(client) != CMD_OK)
                return false;
            return sendString(client, client->tmp);
            client->step++;

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

bool sendString(struct Client * client, char * string)
{
    return false;
}

enum Command recvCommand(struct Client *client)
{
    int ret;

    u_int16_t tmp;

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
    return false;
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

char *recvString(struct Client * client)
{
    return NULL;
}

bool sendCommand(struct Client * client, enum Command cmd)
{
    int ret;

    u_int16_t cmd_n = htonl((u_int16_t) cmd);

    if ((ret = send(client->socket, &cmd_n, sizeof(cmd_n), 0)) != sizeof(cmd_n))
    {
        perror("sendCommand failed");
        return false;    
    }
    
    return true;
}

