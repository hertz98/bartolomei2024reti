#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "server_comm.h"

bool sendMessage(int socket, void * buffer)
{
    if (!sendCommand(socket, CMD_MESSAGE))
        return false;

    if (recvCommand(socket) != CMD_RECVMESSAGE)
        return false;
    
    if (!sendInteger(socket, strlen(buffer)))
        return false;
    
    if (!sendString(socket, buffer, strlen(buffer)))
        return false;

    if (recvCommand(socket) != CMD_OK)
        return false;

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

bool recvMessage(int socket, void * buffer)
{
    if (recvCommand(socket) != CMD_MESSAGE && sendCommand(socket, CMD_RECVMESSAGE))
        return false;

    int size = recvInteger(socket);

    if (!(recvString(socket, (char **) buffer, size)))
        return false;
            
    return sendCommand(socket, CMD_OK);
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

