#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "server_comm.h"

bool sendMessage(int socket, MessageArray * msgs)
{
    if (!msgs)
        return false;

    if (msgs->size > CLIENT_MAX_MESSAGEARRAY_SIZE)
    {
        printf("Errore, messaggio troppo grande\n");
        return false;
    }

    for (int i = -1; i < msgs->size; i++) // Per ciascun messaggio
    {
        Message *msg;
        if (i == -1)
            msg = &msgs->messages[msgs->size]; // L'ultimo messaggio contiene la dimensione del MessageArray
        else
            msg = &msgs->messages[i];
        
        if (msgs->size > CLIENT_MAX_MESSAGE_LENGHT) // Limite HARD CODED
        {
            printf("Errore, messaggio troppo grande\n");
            return false;
        }

        uint32_t lenght = htonl(msg->lenght);

        sendData(socket, &lenght, sizeof(lenght)); // Invio la dimensione del messaggio

        if (msg->lenght && msg->payload)
            sendData(socket, msg->payload, msg->lenght); // Invio il messaggio effettivo
    }

    return recvCommand(socket) == CMD_OK; // Conferma da parte del server
}

bool sendData(int socket, void * buffer, unsigned int lenght)
{
    int ret;
    unsigned int sent = 0;
    
    while(sent < lenght) // Finchè ci sono byte disponibili insisti
    {
        if ((ret = send(socket, buffer + sent, lenght - sent, 0)) >= 0)
            sent += ret;
        else 
            return false; // Se non ha inviato byte errore
    }

    return true;
}

Command inline recvCommand(int socket)
{
    u_int8_t tmp;

    if (recvData(socket, &tmp, sizeof(tmp)))
        return (enum Command) tmp;
    else
        return false;
}

MessageArray * recvMessage(int socket)
{
    uint32_t lenght;
    if (recvCommand(socket) != CMD_MESSAGE)
        return false;

    if (!recvData(socket, &lenght, sizeof(lenght))) // Ricevo la dimensione del MessageArray
        return false;

    lenght = ntohl(lenght);
    MessageArray *tmp = messageArray(lenght); // Allocazione
    if (!tmp){
        printf("impossibile allocare %d bytes", lenght);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < tmp->size; i++) // Per ciascun messaggio
    {
        Message * msg = &tmp->messages[i];

        if (!recvData(socket, &msg->lenght, sizeof(msg->lenght))) // Ricevo la dimensione del messaggio
            return false;
        msg->lenght = ntohl(msg->lenght);

        if (!msg->lenght)
            break;
        
        msg->payload = (void *) malloc(msg->lenght); // Alloco
        if (!msg->payload)
        {
            printf("Errore nella allocazione\n");
            return false;
        }
        
        msg->toFree = true;

        if (!recvData(socket, msg->payload, msg->lenght)) // Ricevo effettivamente il messaggio
            return false;
    }

    sendCommand(socket, CMD_OK);

    return tmp;
}

bool recvData(int socket, void * buffer, unsigned int lenght)
{
    int ret;
    unsigned int received = 0;
    
    while(received < lenght)
    {
        if ((ret = recv(socket, buffer + received, lenght - received, 0)) > 0)
            received += ret;
        else
            return false;
    }

    return true;
}

bool inline sendCommand(int socket, enum Command cmd)
{
    u_int8_t tmp = (u_int8_t) cmd;

    return sendData(socket, &tmp, sizeof(tmp));
}

int client_socketsReady(int * sockets, int size, struct timeval * timeout)
{
    int ret;
    int max = 0;
    fd_set test_fds;

    FD_ZERO(&test_fds);

    for (int i = 0; i < size; i++)
    {
        FD_SET(sockets[i], &test_fds);
        max = sockets[i] > max ? sockets[i] : max;
    }

    if ((ret = select(max + 1, &test_fds, NULL, NULL, timeout)) > 0)
    {
        for (int i = 0; i < size; i++)
            if (FD_ISSET(max, &test_fds))
                return i;
    }

    return -1;
}