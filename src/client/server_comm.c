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
    
    for (int i = -1; i < msgs->size; i++)
    {
        Message *msg;
        if (i == -1)
            msg = &msgs->messages[msgs->size];
        else
            msg = &msgs->messages[i];
        
        uint32_t lenght = htonl(msg->lenght);

        sendData(socket, &lenght, sizeof(lenght));

        if (msg->lenght && msg->data)
            sendData(socket, msg->data, msg->lenght);
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

enum Command recvCommand(int socket)
{
    int ret;

    u_int8_t tmp;

    if ((ret = recv(socket, &tmp, sizeof(tmp), 0) != sizeof(tmp)))
    {
        if (!ret)
            return false;
        if (errno)
            perror("recvCommand failed");
        return false;    
    }

    //client->recv_timestamp = time(NULL);

    return (enum Command) tmp;
}

MessageArray * recvMessage(int socket)
{
    uint32_t lenght;
    if (!recvData(socket, &lenght, sizeof(lenght)))
        return false;

    lenght = ntohl(lenght);
    MessageArray *tmp = messageArray(lenght);

    for (int i = 0; i < tmp->size; i++)
    {
        Message * msg = &tmp->messages[i];

        if (!recvData(socket, &msg->lenght, sizeof(msg->lenght)))
            return false;
        msg->lenght = ntohl(msg->lenght);

        if (!msg->lenght)
            break;
        
        msg->data = (void *) malloc(msg->lenght);

        if (!recvData(socket, &msg->data, msg->lenght))
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

bool sendCommand(int socket, enum Command cmd)
{
    int ret;

    u_int8_t tmp = (u_int8_t) cmd;

    if ((ret = send(socket, &tmp, sizeof(tmp), 0)) != sizeof(tmp))
    {
        if (errno)
            perror("sendCommand failed");
        return false;    
    }
    
    return true;
}

