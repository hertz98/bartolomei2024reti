#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>

#include "../parameters.h"
#include "topic.h"
#include "clients.h"
#include "util.h"
#include "operation.h"
#include "scoreboard.h"
#include "../shared/message.h"

/********** PROTOTIPI DI FUNZIONE **********/

int init(int, char **);
bool clientHandler(ClientsContext * context, int socket);
bool sendHandler(ClientsContext * context, int socket);
void commandHandler();
void printServer();
void signalhandler(int signal);
void exiting();

/********** VARIABILI GLOBALI **********/

int listener;
ClientsContext clientsContext;
TopicsContext topicsContext;

/********** METODI **********/

int main (int argc, char ** argv)
{
    int newfd;            // Newly accepted socket
    int ret;

    if ((ret = init(argc, argv)))
        return ret;

    if ((ret = clientsInit(&clientsContext, MAX_CLIENTs)))
        return ret;

    if ((ret = topicsInit(&topicsContext)))
        return ret;

    if (!topicsLoader(&topicsContext))
        return false;

    if (!scoreboard_init(&clientsContext.scoreboard, &topicsContext))
        return false;
        
    clientsContext.fd_max = listener > STDIN_FILENO ? listener + 1 : STDIN_FILENO + 1;

    while(true)
    {
        printServer();

        fd_set read_fds = clientsContext.master_fds, // Copy the master set
               write_fds = clientsContext.write_fds;
        FD_SET(listener, &read_fds);
        
        // Use select to wait for activity on the sockets
        if (select(clientsContext.fd_max + 1, &read_fds, &write_fds, NULL, &((struct timeval) {0, REFRESH_RATE * 1000})) == -1) {
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
                    operationHandler(&clientsContext, i) != OP_OK)
                    {
                        //ridondante
                        clientsContext.clients[i]->sending = false;
                        FD_CLR(i, &clientsContext.write_fds);
                    }
    }

}

int init(int argc, char ** argv)
{
    listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr)); // Pulizia
    my_addr.sin_family = AF_INET ;

    char * addr;
    switch (argc)
    {
    case 0:
    case 1:
        addr = DEFAULT_BIND_IP;
        my_addr.sin_port = htons( DEFAULT_BIND_PORT );
        break;
    case 2:
        addr = DEFAULT_BIND_IP;
        my_addr.sin_port = htons( atoi( argv[1] ));
        break;
    case 3:
    default:
        addr = argv[1];
        my_addr.sin_port = htons( atoi( argv[2] ));
        break;
    }

    if (inet_pton(AF_INET, addr, &my_addr.sin_addr) == -1)
    {
        printf("Errore nella conversione dell'indirizzo ip\n");
        return 1;
    }

#ifdef DEBUG
    
    int opt = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

#endif

    if (bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1)
    {
        perror("Errore nella bind");
        return 1;
    }

    if (listen(listener, MAX_CLIENTs) == -1) // Massimo numero di clients prima di scegliere un nome
    {
        perror("Errore nella listen");
        return 1;
    }

    atexit(exiting);
    signal(SIGINT, signalhandler);
    signal(SIGPIPE, SIG_IGN);

    srand(time(NULL));

    return 0;
}

bool clientHandler(ClientsContext * context, int socket)
{
    struct Client * client = context->clients[socket];

    if (client->sending) // Il client non è in sync con il server, termina
        return false;
    
    if(client->operation)
        return operationHandler(context, socket);

    if (!client->registered)
    {
        if (recvCommand(socket) == CMD_REGISTER)
        {
            sendCommand(socket, CMD_OK);
            return operationCreate(regPlayer, context, socket, &topicsContext);
        }
        else
            return false;
    }

    switch (recvCommand(socket))
    {
    case false: // CMD_STOP
        return false;
        break;
    
    case CMD_TOPICS:
        return client_sendTopics(context, socket, &topicsContext);
    
    case CMD_TOPICS_PLAYABLE:
        return client_sendPlayable(&clientsContext, &topicsContext, socket);

    case CMD_SELECT:
        return operationCreate(selectTopic, context, socket, &topicsContext);

    case CMD_RANK:
        return client_sendScoreboard(&clientsContext, socket);

    case CMD_NEXTQUESTION:
        if (client->game.playing < 0 || client->game.currentQuestion < 0)
            sendCommand(socket, CMD_NONE);
        else
            return operationCreate(playTopic, context, socket, &topicsContext);
        break;
    
    default:
        break;
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
    
    printf("Partecipanti (%d)\n", clientsContext.registered);
    for (int i = 0, n = 0; i < clientsContext.allocated && n < clientsContext.nClients; i++)
        if (isClient(&clientsContext, i, true))
        {
            printf("- %s\n", clientsContext.clients[i]->name);
            n++;
        }
    printf("\n");

    Scoreboard * scoreboard = &clientsContext.scoreboard;

    scoreboard_serialize_update(scoreboard, &topicsContext);

    for (int i = 0; i < SCOREBOARD_SIZE * scoreboard->nTopics; i++)
        printf("%s\n", scoreboard->serialized[i].string);

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
    clientsFree(&clientsContext);
    topicsFree(&topicsContext);
    scoreboard_destroy(&clientsContext.scoreboard);
}