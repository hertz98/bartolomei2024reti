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
struct sockaddr_in my_addr;

#define MAX_CONNECTED 32
#define MAX_CLIENTs 32
int clients[MAX_CLIENTs];
int n_clients = 0;

#define BUFFER_SIZE 20
char buffer[BUFFER_SIZE + 1];

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
        close(clients[i]);
    close(listener);
    printf("\nSocket chiusi\n");
    exit(0);
}

int init(int, char **);
bool clientHandler(int);

int main (int argc, char ** argv)
{
    fd_set master,  // Main set of file descriptors
        read_fds;   // Set for reading
    int fdmax;            // Maximum number of file descriptors
    struct sockaddr_in sv_addr; // Server address
    struct sockaddr_in cl_addr; // Client address
    int newfd;            // Newly accepted socket
    int nbytes, addrlen, i, ret;

    if (ret = init(argc, argv))
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
                    addrlen = sizeof(cl_addr);
                    if ((newfd = accept(listener, (struct sockaddr *)&cl_addr, &addrlen)) < 0) 
                    {
                        perror("Accept fallita");
                        continue;
                    }
                    FD_SET(newfd, &master); // Add the new socket to the master set
                    if (newfd > fdmax)
                        fdmax = newfd; // Update the max file descriptor          
                } 
                else
                    if (!clientHandler(i))
                    {
                        close(i);           // Close the socket
                        FD_CLR(i, &master); // Remove from the master set
                    }

            }

        time_t rawtime;
        struct tm * timeinfo;
        time(&rawtime);
        timeinfo = localtime (&rawtime);
        if (!strftime(buffer, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S", timeinfo))
        {
            printf("Dimensione buffer insufficiente, termino..");
            return(1);
        }
        printf("%s\n", buffer);

    }

}

int init(int argc, char ** argv)
{
    if (argc < 3)
    {
        printf("Argomenti insufficienti\n");
        return(1);
    }

    buffer[BUFFER_SIZE] = '\0';

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

    if (listen(listener, MAX_CONNECTED) == -1)
    {
        perror("Errore nella listen");
        return 1;
    }

    atexit(socketclose);
    signal(SIGINT, signalhandle);

    return 0;
}

bool clientHandler(int i)
{
    if (recv(i, buffer, BUFFER_SIZE, 0) <= 0) 
    {
        perror("recv fallita");
        return false;
    }
    else
    {
        if (!strncmp(buffer, "REGISTER", BUFFER_SIZE))
        {
            strncpy(buffer, "REGISTERED", BUFFER_SIZE);
            if (send(i, &buffer, BUFFER_SIZE, 0) == BUFFER_SIZE)
            {
                clients[n_clients++] = i;
                printf("Client registrato\n");
                return true;
            }
            else
            {
                perror("Send REGISTERED fallita");
                return false;
            }
        }
    }
    printf("Client ha mandato spazzatura: %s\n", buffer);
    return true;
}