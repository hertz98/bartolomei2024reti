#define _DEFAULT_SOURCE
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
#include "util.h"
#include "../shared/message.h"

#define DEBUG

#define MAX_CLIENTs 32
#define SLEEP_TIME 1
#define DATA_DIR "./data/"

void signalhandler(int signal);
void exiting();
void closeSockets(void * p);

int init(int, char **);
bool clientHandler(ClientsContext * context, int socket);
bool sendHandler(ClientsContext * context, int socket);
void commandHandler();
void printServer();

int listener;
ClientsContext clientsContext;
TopicsContext topicsContext;

int main (int argc, char ** argv)
{
    int newfd;            // Newly accepted socket
    int ret;

    if ((ret = init(argc, argv)))
        return ret;

    if ((ret = clientsInit(&clientsContext, MAX_CLIENTs)))
        return ret;

    if ((ret = topicsInit(&topicsContext, DATA_DIR)))
        return ret;

    if (!topicsLoader(&topicsContext))
        return false;

    clientsContext.fd_max = listener > STDIN_FILENO ? listener + 1 : STDIN_FILENO + 1;

    while(true)
    {
        printServer();

        struct timeval timeout = {SLEEP_TIME, 0};

        fd_set read_fds = clientsContext.master_fds, // Copy the master set
               write_fds = clientsContext.write_fds;
        FD_SET(listener, &read_fds);
        
        // Use select to wait for activity on the sockets
        if (select(clientsContext.fd_max + 1, &read_fds, &write_fds, NULL, &timeout) == -1) {
            perror("Select failed");
            exit(1);
        }

        // Loop through the file descriptors to check activity
        for (int i = 0; i <= clientsContext.fd_max; i++) 
            if (FD_ISSET(i, &read_fds)) // Found a ready descriptor
            { 
                if (i == listener) // Listener socket is ready
                { 
                    struct sockaddr_in cl_addr;
                    unsigned int addrlen = sizeof(cl_addr);
                    if ((newfd = accept(listener, (struct sockaddr *)&cl_addr, &addrlen)) < 0) 
                    {
                        perror("Accept fallita");
                        continue;
                    }
                    clientAdd(&clientsContext, newfd);
                    printf("registering\n");    
                } 
                else
                    if (!clientHandler(&clientsContext, i))
                        clientRemove(&clientsContext, i);
            }

        for (int i = 0; i <= clientsContext.fd_max; i++) 
            if (FD_ISSET(i, &write_fds)) // Found a ready descriptor
                if(isClient(&clientsContext, i, false) &&
                    sendMessageHandler(&clientsContext, i) != OP_OK)
                    {
                        clientsContext.clients[i]->toSend = NULL; // TODO: free
                        FD_CLR(i, &clientsContext.write_fds);
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

    struct sockaddr_in my_addr;
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

    atexit(exiting);
    signal(SIGINT, signalhandler);
    signal(SIGPIPE, SIG_IGN);

    return 0;
}

bool clientHandler(ClientsContext * context, int socket)
{
    struct Client * client = context->clients[socket];

    if (client->toSend != NULL) // Il client non è in sync con quello del server, termina
        return false;
    
    if (client->operation[0].function)
        return client->operation[0].function(context, socket, client->operation[0].p);

    if (!client->registered)
    {
        if (recvCommand(socket) == CMD_REGISTER)
        {
            sendCommand(socket, CMD_OK);
            return recvMessage(context, socket, &client->name);
            //return regPlayer(context, socket, &topicsContext, true);
        }
        else
            return false;
    }

    enum Command cmd = recvCommand(socket);

    if (cmd == false) // false
        return false;

    if (cmd == CMD_TOPICS)
    {
        sendCommand(socket, CMD_OK);
        MessageArray * msgs = messageArray(1);
        messageString(&msgs->messages[0], topicsContext.topicsString, false);
        sendMessage(context, socket, (void *) msgs);
    }

    return true;
}

void printServer()
{
    printf("\033[H\033[J"); // system("clear");
    printf("Trivia Quiz\n+++++++++++++++++++++++++++++++\n");
    
    printf("Temi:\n");
    for (int i = 0; i < topicsContext.nTopics; i++)
        printf("%d - %s\n", i + 1, topicsContext.topics[i].name);

    printf("+++++++++++++++++++++++++++++++\n\n");
    
    printf("Partecipanti (%d)\n", clientsContext.nClients);
    for (int i = 0, n = 0; i < clientsContext.allocated && n < clientsContext.nClients; i++)
        if (isClient(&clientsContext, i, true))
        {
            printf("- %s\n", clientsContext.clients[i]->name);
            n++;
        }
    printf("\n");

    for (int t = 0; t < topicsContext.nTopics; t++)
    {
        printf("Punteggio tema %d\n", t + 1);
        for (int i = 0, n = 0; i < clientsContext.allocated && n < clientsContext.nClients; i++)
            if (isClient(&clientsContext, i, true) &&
                clientsContext.clients[i]->game.playing == t &&
                clientsContext.clients[i]->game.score[t] != -1)
            {
                printf("- %s %d\n", clientsContext.clients[i]->name, clientsContext.clients[i]->game.score[t]);
                n++;
            }
        printf("\n");
    }
    
    for (int t = 0; t < topicsContext.nTopics; t++)
    {
        printf("Quiz tema %d completato\n", t + 1);
        for (int i = 0, n = 0; i < clientsContext.allocated && n < clientsContext.nClients; i++)
            if (isClient(&clientsContext, i, true) &&
                clientsContext.clients[i]->game.playing != t &&
                clientsContext.clients[i]->game.score[t] != -1)
            {
                printf("- %s\n", clientsContext.clients[i]->name);
            }
        printf("\n");
    }
}

void signalhandler(int signal)
{
    if (signal == SIGINT)
    {
        printf("\nCTRIL+C:\n");
    }
    exit(0);
}

void exiting()
{
    closeSockets(NULL);
}

void closeSockets(void * p)
{
    clientsFree(&clientsContext);
    topicsFree(&topicsContext);
}