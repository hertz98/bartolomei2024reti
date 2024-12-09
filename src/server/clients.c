#include <string.h>
#include <stdlib.h>
#include "clients.h"

bool clientAdd(fd_set * master, struct Client * clients, int socket)
{
    struct Client * client = (struct Client *) malloc(sizeof(struct Client));

    if (! client)
        return false;

    memset(clients, 0, sizeof(struct Client));

    client->socket = socket;
    client->name[0] = '\0';
    client->operation = NULL;
    client->recv_timestamp = time(NULL);

    FD_SET(socket, master);

    return true;
}

void clientRemove(fd_set * master, struct Client * client, int socket)
{
    if (FD_ISSET(socket, master))
        free(client);

    FD_CLR(socket, master);
}
