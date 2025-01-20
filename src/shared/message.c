#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "message.h"

void messageArrayDestroy(MessageArray *messageArray, void (*destroyer)(void *payload))
{
    if (!messageArray)
        return;

    if (messageArray->messages)
    {
        for (int i = 0; i <= messageArray->size; i++)
        {
            Message *msg = &messageArray->messages[i];
            if (msg->toFree)
            {
                if (destroyer)
                    destroyer(msg->data);
                else
                    free(msg->data);
            }
        }
        free(messageArray->messages);
    }

    free(messageArray);
}

MessageArray *messageArray(int size)
{
    if (size < 0)
        return NULL;
    MessageArray * tmp = (MessageArray *) malloc(sizeof(MessageArray));
    if (!tmp)
        return NULL;
    tmp->size = size;
    tmp->messages = (Message *) malloc(sizeof(Message) * (size + 1));
    if (!tmp->messages)
    {
        free(tmp);
        return NULL;
    }
    memset(tmp->messages, 0, sizeof(Message) * (size + 1));
    tmp->messages[size].lenght = size;
    return tmp;
}

void messageString(Message *message, char *string, bool toFree)
{
    if (!string)
        return;

    message->data = string;
    message->lenght = strlen(string) + 1;
    message->transmitted = 0;
    message->toFree = toFree;
    
    return;
}

void messageInteger(Message *message, int32_t number)
{
    if (!message)
        return;
    
    message->data = malloc(sizeof(number));
    if (!message->data)
    {
        printf("Error allocating space");
        exit(EXIT_FAILURE);
    }
    *(int32_t *) message->data = htonl(number);
    message->lenght = sizeof(number);
    message->transmitted = 0;
    message->toFree = true;
}

void emptyMessage(Message *message)
{
    message->data = NULL;
    message->lenght = 0;
    message->transmitted = 0;
    message->toFree = false;

    return;
}
