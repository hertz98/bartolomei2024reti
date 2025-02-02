#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "message.h"

void messageArrayDestroy(MessageArray **messageArray)
{
    if (!messageArray || !*messageArray)
        return;

    if ((*messageArray)->messages)
    {
        for (int i = 0; i <= (*messageArray)->size; i++)
        {
            Message *msg = &(*messageArray)->messages[i];
            if (msg->toFree)
                free(msg->payload);
        }
        free((*messageArray)->messages);
    }

    free(*messageArray);

    *messageArray = NULL; // Lascio il puntatore a NULL per evitare altre deallocazioni indesiderate
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

char ** messageArray2StringArray(MessageArray *messageArray)
{

    char ** tmp = malloc(sizeof(char*) * messageArray->size);
    if (!tmp)
    {
        printf("Error allocating space\n");
        return NULL;
    }

    for (int i = 0; i < messageArray->size; i++){
        messageArray->messages[i].toFree = false;
        tmp[i] = (char *) messageArray->messages[i].payload;
    }

    return tmp;
}

void messageString(Message *message, char *string, bool toFree)
{
    if (!string)
        return;

    message->payload = string;
    message->lenght = strlen(string) + 1;
    message->transmitted = 0;
    message->toFree = toFree;
    
    return;
}

bool messageInteger(Message *message, int32_t number)
{
    if (!message)
        return false;;
    
    message->payload = malloc(sizeof(number));
    if (!message->payload)
    {
        printf("Error allocating space\n");
        return false;
    }
    *(int32_t *) message->payload = htonl(number);
    message->lenght = sizeof(number);
    message->transmitted = 0;
    message->toFree = true; // In questo caso porto a true perché ho eseguito una copia

    return true;
}

void messageBoolArray(Message * message, bool *array, int size)
{
    if (!message)
        return;
    
    if (size <= 0)
    {
        message->payload = NULL;
        message->lenght = 0;
        message->toFree = false;
        return;
    }

    message->lenght = sizeof(bool) * size;
    message->payload = array; 

    message->transmitted = 0;
    message->toFree = false;
}

bool messageIntegerArray(Message *message, int32_t *array, int size)
{
    if (!message)
        return false;
    
    if (size <= 0)
    {
        message->payload = NULL;
        message->lenght = 0;
        message->toFree = false;
        return true;
    }

    message->lenght = sizeof(uint32_t) * size;
    message->payload = malloc(message->lenght);
    if (!message->payload)
    {
        printf("Error allocating space\n");
        return false;
    }

    for (int i = 0; i < size; i++)
        ((int *) message->payload)[i] = htonl(array[i]);

    message->transmitted = 0;
    message->toFree = true; // In questo caso porto a true perché ho eseguito una copia

    return true;
}

void inline emptyMessage(Message *message)
{
    memset(message, 0, sizeof(Message));
}
