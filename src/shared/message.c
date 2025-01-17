#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "message.h"

Message *messageString(char *string, bool toFree)
{
    if (!string)
        return NULL;

    Message * tmp = (Message *) malloc( sizeof(Message) );
    tmp->data = string;
    tmp->lenght = strlen(string) + 1;
    tmp->transmitted = 0;
    tmp->toFree = toFree;
    tmp->next = NULL;
    
    return tmp;
}

Message *emptyMessage()
{
    Message * tmp = (Message *) malloc( sizeof(Message) );
    tmp->data = NULL;
    tmp->lenght = 0;
    tmp->transmitted = 0;
    tmp->toFree = false;
    tmp->next = NULL;

    return tmp;
}