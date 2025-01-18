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

bool sendInteger(int socket, int i)
{
    int ret;

    u_int32_t tmp = htonl(i);

    if ((ret = send(socket, &tmp, sizeof(tmp), 0)) != sizeof(tmp))
    {
        perror("sendInteger failed");
        return false;    
    }
    
    return true;
}

bool sendString(int socket, char * buffer, int lenght)
{
    int ret;

    if ((ret = send(socket, buffer, lenght, 0)) != lenght)
        return false;

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
    int ret;

    MessageArray * tmp;

    ret = 0;
    int lenght;
    while(ret < sizeof(uint32_t))
    {
        ret += recv(socket, &lenght + ret, sizeof(uint32_t) - ret, 0);
        if (!ret)
            return false;
    }
    lenght = ntohl(lenght);
    tmp = messageArray(lenght);

    for (int i = 0; i < tmp->size; i++)
    {
        Message * msg = &tmp->messages[i];

        ret = 0;
        while(ret < sizeof(msg->lenght))
        {
            ret += recv(socket, &msg->lenght + ret, sizeof(msg->lenght) - ret, 0);
            if (!ret)
                return false;
        }
        msg->lenght = ntohl(msg->lenght);

        if (!msg->lenght)
            break;
        
        msg->data = (void *) malloc(msg->lenght);

        ret = 0;
        while(ret < msg->lenght)
        {
            ret += recv(socket, msg->data + ret, msg->lenght - ret, 0);
            if (!ret)
                return false;
        }
    }

    sendCommand(socket, CMD_OK);

    return tmp;
}

bool recvString(int socket, char ** buffer, int lenght)
{
    int ret;

    *buffer = (char *) malloc(sizeof(char) * lenght);
    if (!buffer)
        return false;

    if ((ret = recv(socket, *buffer, lenght, 0)) != lenght)
    {
        free(*buffer);
        *buffer = NULL;

        if (!ret)
            return false;
        
        if (errno)
            perror("recvString failed");

        return false;
    }

    return true;
}

int recvInteger(int socket)
{
    int ret;

    u_int32_t tmp;

    if ((ret = recv(socket, &tmp, sizeof(tmp), 0) != sizeof(tmp)))
    {
        if (!ret) // Sever disconnesso
            return false;
        
        if (errno)
            perror("recvInteger failed");
        return false;    
    }

    return ntohl(tmp);
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

