#include <string.h>
#include <arpa/inet.h>
#include "util.h"
#include "operation.h"
#include "clients.h"

bool operationHandler(ClientsContext *context, int socket)
{
    Client *client = context->clients[socket];

    if (!client->operation)
        return !client->nOperations ? true : false;

    if (client->nOperations > MAX_OPERATIONS_PER_CLIENT)
        return false;

    while(client->operation)
    {
        Operation * currentOperation = client->operation->data;

        OperationResult ret; 
        switch (ret = (*currentOperation->function)(context, socket, currentOperation->p))
        {
            case OP_OK:
                if (socketReady(socket)) // Se ci sono messaggi disponibili finisci a servire il client
                    continue;
                else
                    return true; // Servi più tardi
            case OP_DONE:
                operationDestroy( list_extractHead(&client->operation)->data );
                client->nOperations--;
                continue;
            case OP_FAIL:
            default:
                return false; // Rimuovi il client
        }
    }

    return true; // In questo caso nessuna operazione è in corso, ritorno true per non rimuovere il client
}

bool socketReady(int socket)
{
    fd_set test_fds;
    FD_ZERO(&test_fds);
    FD_SET(socket, &test_fds);
    if (select(socket + 1, &test_fds, NULL, NULL, &(struct timeval) {0,0} ) > 0)
        return true;
    return false;
}

bool operationCreate(OperationResult (*function)(ClientsContext *context, int socket, void *), ClientsContext *context, int socket, void *p)
{
    Client * client = context->clients[socket];

    // TODO: controllo numero operazioni
    
    Operation * tmp = malloc(sizeof(Operation));
    if (!tmp)
        return OP_FAIL;

    memset(tmp, 0, sizeof(Operation));
    
    tmp->function = function;
    tmp->p = p;
    tmp->step = 0;

    if (list_insertHead(&client->operation, tmp))
    {
        client->nOperations++;
        return operationHandler(context, socket);
    }
    else
    {
        free(tmp);
        return OP_FAIL;
    }

    return false;
}

void operationDestroy(void *operation)
{
    if (!operation)
        return;

    if ( ((Operation *) operation)->function == recvMessage)
    {

    }

    free(operation);
}

OperationResult regPlayer(ClientsContext *context, int socket, void *topicsContext)
{
    Client *client = context->clients[socket];
    TopicsContext * topics = topicsContext;

    Operation *currentOperation = client->operation->data;

    switch (currentOperation->step++)
    {
    case 0:
        return operationCreate(recvMessage, context, socket, &client->name);

    case 1:
        MessageArray * tmp = (void*) client->name;
        client->name = (char*) ((MessageArray *) client->name)->messages->payload;
        messageArrayDestroy(&tmp);

        enum Command ret;
        if ((ret = nameValid(context, socket, client->name)) == CMD_OK)
        {
            if (client_gameInit(client, topics) && sendCommand(socket, CMD_OK))
            {
                context->registered++;
                client->registered = true;
                return OP_DONE;
            }
        }
        else
        {
            free(client->name);
            client->name = NULL;
            if (!sendCommand(socket, ret))
                return OP_FAIL;
            return OP_DONE;
        }
    
    default:
        break;
    }

    return OP_FAIL;
}

OperationResult selectTopic(ClientsContext *context, int socket, void * topicsContext)
{
    Client *client = context->clients[socket];
    TopicsContext * topics = topicsContext;

    Operation *currentOperation = client->operation->data;

    switch (currentOperation->step++)
    {
    case 0:
        currentOperation->tmp = messageArray(client->game.nPlayable);
        for (int t = 0, m = 0; t < topics->nTopics; t++)
            if (client->game.playableTopics[t])
                messageString( &( ((MessageArray *)currentOperation->tmp)->messages[m++] ), topics->topics[t].name, false);
        currentOperation->step++;

    case 1:
        return operationCreate(sendMessage, context, socket, currentOperation->tmp);
    
    case 2:
        return OP_OK;

    case 3:
        return operationCreate(recvMessage, context, socket, &currentOperation->tmp);
    
    case 4:
        client->game.playing = ntohl( *(int32_t*) ((MessageArray *)currentOperation->tmp)->messages[0].payload); //TODO: Free()???
        client->game.playing = client_playableIndex(client, topics, client->game.playing);
        
        if (client->game.playing < 0 || !client_setPlayed(client, topics, client->game.playing)) // TODO: Distinzione errore nel server da input scorretto
        {
            sendCommand(socket, CMD_NOTVALID);
            return OP_FAIL;
            break;
        }
        client->game.currentQuestion = 0;
        if (!client_quizInit(client, topics))
            return OP_FAIL;
        return OP_DONE;

    default:
        break;
    }

    return OP_FAIL;
}

OperationResult sendMessage(ClientsContext *context, int socket, void *message_array)
{
    Client *client = context->clients[socket];

    Operation *currentOperation = client->operation->data;

    MessageArray *msgs = (MessageArray *) message_array;

    switch (currentOperation->step++)
    {
        case 0:
            if (sendCommand(socket, CMD_MESSAGE))
            {
                client->toSend = msgs;
                client->transferring = -1;
                FD_SET(socket, &context->write_fds);   
                return OP_OK;
            }
            break;

        case 1:
            if (recvCommand(socket) == CMD_OK)
                return OP_DONE;
            break;
    }

    return OP_FAIL;
}

OperationResult recvMessage(ClientsContext *context, int socket, void *pointer)
{
    Client * client = context->clients[socket];
    OperationResult ret = OP_OK;

    Operation *currentOperation = client->operation->data;

    MessageArray ** msgs = (MessageArray **) pointer;

    switch (currentOperation->step)
    {
    case 0:
        *msgs = messageArray(0);
        client->transferring = -1;
        currentOperation->step++;

    case 1:
        if (client->transferring == -1)
        {
            Message *tmp = &(*msgs)->messages[0];
            if ((ret = recvData(socket, &tmp->lenght, sizeof(tmp->lenght), &tmp->transmitted)) == OP_DONE)
            {
                int size = ntohl(tmp->lenght);
                messageArrayDestroy((MessageArray **) msgs);
                *msgs = messageArray(size);
                client->transferring = 0;
            }
            return (bool) ret;
        }

        while (client->transferring < (*msgs)->size)
        {
            Message * msg = &(*msgs)->messages[client->transferring];

            if (msg->transmitted < sizeof(msg->lenght))
            {
                if ((ret = recvData(socket, &msg->lenght, sizeof(msg->lenght), &msg->transmitted)) == OP_DONE)
                    {
                        msg->lenght = ntohl(msg->lenght);
                        msg->payload = (void *) malloc(msg->lenght + 1);
                    }
                else
                    return (bool) ret;
            }

            if (msg->lenght)
            {
                unsigned int sent = msg->transmitted - sizeof(msg->lenght);

                if ((ret = recvData(socket, msg->payload, msg->lenght, &sent)) == OP_DONE)
                    *(char*) (msg->payload + msg->lenght) = '\0'; // Proteggo le stringhe ponendo il byte (in più) nullo
                else
                    return (bool) ret;
            }

            client->transferring++;
        }

        currentOperation->step++;

    case 2:
        return sendCommand(socket, CMD_OK) ? OP_DONE : OP_FAIL;

    default:
        break;
    }
    
    return OP_FAIL;
}


OperationResult playTopic(ClientsContext *context, int socket, void *topicsContext)
{
    Client *client = context->clients[socket];
    TopicsContext * topics = topicsContext;

    Operation *currentOperation = client->operation->data;

    if (!client->registered)
        return OP_FAIL;

    if (client->game.playing < 0 || client->game.currentQuestion < 0) // ridondante
    {
        if (!sendCommand(socket, CMD_NONE))
            return OP_FAIL;
        return OP_DONE;
    }

    Topic *currentTopic = &topics->topics[client->game.playing];
    Question *currentQuestion = client->game.questions[client->game.currentQuestion];

    switch (currentOperation->step++)
    {
    case 0:
        if (client->game.playing < 0 || client->game.currentQuestion < 0)
        {
            if (sendCommand(socket, CMD_NONE))
                return OP_DONE;
            else return OP_FAIL;
        }
        else
        {
            if (sendCommand(socket, CMD_OK))
            {
                MessageArray *question_msg = messageArray(1);
                messageString(&question_msg->messages[0], currentQuestion->question, false);
                currentOperation->tmp = question_msg;
            }
            else return OP_FAIL;
        }
        currentOperation->step++;

    case 1:
        MessageArray *question_msg = currentOperation->tmp;
        return operationCreate(sendMessage, context, socket, question_msg);
    
    case 2:
        return OP_OK;

    case 3:
        return operationCreate(recvMessage, context, socket, &currentOperation->tmp);

    case 4:
        MessageArray *answer_msg = currentOperation->tmp;
        
        if (!stricmp(answer_msg->messages[0].payload, currentQuestion->answer)) // TODO: Custom answer compare
        {
            if (!sendCommand(socket, CMD_CORRECT))
                return OP_FAIL;
            client->game.score[client->game.playing]++;
        }
        else
            if (!sendCommand(socket, CMD_WRONG))
                return OP_FAIL;

        if (++client->game.currentQuestion >= currentTopic->nQuestions)
        {
            client->game.playing = -1;
            client->game.currentQuestion = -1;
        }

        return OP_DONE;

    default:
        break;
    }

    return OP_FAIL;
}