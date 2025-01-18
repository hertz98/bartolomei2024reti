#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "message.h"

MessageArray *messageArray(int size)
{
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

void emptyMessage(Message *message)
{
    message->data = NULL;
    message->lenght = 0;
    message->transmitted = 0;
    message->toFree = false;

    return;
}
