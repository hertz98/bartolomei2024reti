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

int listener;
fd_set master;
struct sockaddr_in my_addr;

#define MAX_CLIENTs 32
struct Client * clients[MAX_CLIENTs];
int n_clients = 0;

#define SLEEP_TIME 10

void signalhandle(int signal)
{
    printf("\nCTRIL+C:\n");
    exit(0);
}

void socketclose()
{ 
    int i;
    for (i = 0; i < n_clients; i++)
        close(clients[i]->socket);
    close(listener);
    printf("\nSocket chiusi\n");
    exit(0);
}

int init(int, char **);
bool clientHandler(struct Client * client);

int main (int argc, char ** argv)
{
    fd_set read_fds;   // Set for reading
    int fdmax;            // Maximum number of file descriptors
    struct sockaddr_in cl_addr; // Client address
    int newfd;            // Newly accepted socket
    int i, ret;

    if ((ret = init(argc, argv)))
        return ret;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    // Add the listener to the master set
    FD_SET(listener, &master);
    fdmax = listener; // Keep track of the maximum file descriptor

    while(true)
    {

        struct timeval timeout = {10, 0};

        read_fds = master; // Copy the master set

        // Use select to wait for activity on the sockets
        if (select(fdmax + 1, &read_fds, NULL, NULL, &timeout) == -1) {
            perror("Select failed");
            exit(1);
        }

        // Loop through the file descriptors to check activity
        for (i = 0; i <= fdmax; i++) 
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
                    clientAdd(&master, clients, i);
                    
                    FD_SET(newfd, &master); // Add the new socket to the master set
                    if (newfd > fdmax)
                        fdmax = newfd; // Update the max file descriptor          
                } 
                else
                    if (!clientHandler(clients[i]))
                    {
                        close(i);           // Close the socket
                        FD_CLR(i, &master); // Remove from the master set
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

    atexit(socketclose);
    signal(SIGINT, signalhandle);

    return 0;
}



bool clientHandler(struct Client * client)
{
    if (client->operation != NULL)
        return (*client->operation)(client, NULL);
    
    // Client non ancora registrato: ha stringa nulla
    if (client->name[0] == '\0')
    {
        if (recvCommand(client) == CMD_REGISTER)
        {
            return recvMessage(client, client->name);
        }
        else
            return false;
    }

    return true;
}