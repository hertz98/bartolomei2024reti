#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>

#include "topic.h"
#include "clients.h"

#define DEBUG

int listener;
struct sockaddr_in my_addr;

#define MAX_CLIENTs 32
//struct Client * clients[MAX_CLIENTs];
//int n_clients = 0;

#define SLEEP_TIME 10

void signalhandle(int signal)
{
    printf("\nCTRIL+C:\n");
    exit(0);
}

void socketclose()
{ 

}

int init(int, char **);
bool clientHandler(ClientsContext * context, int socket);

int main (int argc, char ** argv)
{
    ClientsContext * clientsContext;

    fd_set read_fds, *master;   // Set for reading
    struct sockaddr_in cl_addr; // Client address
    int newfd;            // Newly accepted socket
    int i, ret;

    if ((ret = init(argc, argv)))
        return ret;

    if ((ret = clientsInit(&clientsContext, MAX_CLIENTs)))
            return ret;
    master = &clientsContext->master;

    FD_ZERO(&read_fds);

    // Add the listener to the master set
    FD_SET(listener, master);
    clientsContext->fd_max = listener; // Keep track of the maximum file descriptor

    while(true)
    {

        struct timeval timeout = {1, 0};

        read_fds = *master; // Copy the master set

        // Use select to wait for activity on the sockets
        if (select(clientsContext->fd_max + 1, &read_fds, NULL, NULL, &timeout) == -1) {
            perror("Select failed");
            exit(1);
        }

        // Loop through the file descriptors to check activity
        for (i = 0; i <= clientsContext->fd_max; i++) 
            if (FD_ISSET(i, &read_fds)) // Found a ready descriptor
            { 
                if (i == listener) // Listener socket is ready
                { 
                    unsigned int addrlen = sizeof(cl_addr);
                    if ((newfd = accept(listener, (struct sockaddr *)&cl_addr, &addrlen)) < 0) 
                    {
                        perror("Accept fallita");
                        continue;
                    }
                    clientAdd(clientsContext, newfd);
                    printf("registering\n");    
                } 
                else
                    if (!clientHandler(clientsContext, i))
                    {
                        clientRemove(clientsContext, i);
                        printf("removed\n");
                    }
            }

    }

}

int init(int argc, char ** argv)
{
    if (argc < 3)
    {
        printf("Argomenti insufficienti\n");
        return(1);
    }

    //buffer[BUFFER_SIZE] = '\0';

    listener = socket(AF_INET, SOCK_STREAM, 0);

    memset(&my_addr, 0, sizeof(my_addr)); // Pulizia
    my_addr.sin_family = AF_INET ;
    my_addr.sin_port = atoi( argv[2] );
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (inet_pton(AF_INET, argv[1], &my_addr.sin_addr) == -1)
    {
        printf("Errore nella conversione dell'indirizzo ip\n");
        return 1;
    }

    if (bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1)
    {
        perror("Errore nella bind");
        return 1;
    }

    if (listen(listener, MAX_CLIENTs) == -1)
    {
        perror("Errore nella listen");
        return 1;
    }

#ifdef DEBUG
    
    int opt = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

#endif

    atexit(socketclose);
    signal(SIGINT, signalhandle);

    return 0;
}



bool clientHandler(ClientsContext * context, int socket)
{
    struct Client * client = context->clients[socket];

    if (client->operation != NULL)
        return (*client->operation)(client, NULL, false);
    
    if (!client->registered)
    {
        if (recvCommand(client) == CMD_REGISTER)
        {
            sendCommand(client, CMD_OK);
            return regPlayer(client, context, true);
        }
        else
            return false;
    }

    enum Command cmd = recvCommand(client);

    if (cmd == false) // false
        return false;

    printf("%s\n",client->name);

    return true;
}