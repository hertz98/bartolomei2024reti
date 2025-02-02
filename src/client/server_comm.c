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
    
    if (msgs->size > SEND_MAX_MESSAGEARRAY_SIZE)
    {
        printf("Errore, messaggio troppo grande\n");
        return false;
    }

    for (int i = -1; i < msgs->size; i++)
    {
        Message *msg;
        if (i == -1)
            msg = &msgs->messages[msgs->size];
        else
            msg = &msgs->messages[i];
        
        if (msgs->size > SEND_MAX_MESSAGE_LENGHT)
        {
            printf("Errore, messaggio troppo grande\n");
            return false;
        }

        uint32_t lenght = htonl(msg->lenght);

        sendData(socket, &lenght, sizeof(lenght));

        if (msg->lenght && msg->payload)
            sendData(socket, msg->payload, msg->lenght);
    }

    return recvCommand(socket) == CMD_OK;
}

bool sendData(int socket, void * buffer, unsigned int lenght)
{
    int ret;
    unsigned int sent = 0;
    
    while(sent < lenght)
    {
        if ((ret = send(socket, buffer + sent, lenght - sent, 0)) >= 0)
            sent += ret;
        else
            return false;
    }

    return true;
}

enum Command inline recvCommand(int socket)
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

    if (!recvData(socket, &lenght, sizeof(lenght)))
        return false;

    lenght = ntohl(lenght);
    MessageArray *tmp = messageArray(lenght);

    if (!tmp){
        printf("impossibile allocare %d bytes", lenght);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < tmp->size; i++)
    {
        Message * msg = &tmp->messages[i];

        if (!recvData(socket, &msg->lenght, sizeof(msg->lenght)))
            return false;
        msg->lenght = ntohl(msg->lenght);

        if (!msg->lenght)
            break;
        
        msg->payload = (void *) malloc(msg->lenght);
        if (!msg->payload)
        {
            printf("Errore nella allocazione\n");
            return false;
        }
        
        msg->toFree = true;

        if (!recvData(socket, msg->payload, msg->lenght))
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

