#define _DEFAULT_SOURCE
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
                Node * tmp = list_extractHead(&client->operation);
                operationDestroy(tmp->data);
                free(tmp);
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
    tmp->tmp = NULL;
    tmp->count = 0;

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

    Operation * currentOperation = operation;

    // Nel caso in cui si ricevono dei dati è necessario deallocarle anche quando le operazioni non arrivano a fine
    if ( currentOperation->function == selectTopic 
        || currentOperation->function == playTopic)
    {
        messageArrayDestroy((MessageArray **) &currentOperation->tmp);
    }
    if (currentOperation->function == sendMessage)
    {
        messageArrayDestroy((MessageArray **)  &currentOperation->p);
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
        tmp->messages[0].toFree = false;
        client->name = (char*) tmp->messages[0].payload;
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
    case 0: // invio la lista dei giochi disponibili al client
        MessageArray *tmp = messageArray(1);
        messageBoolArray( &((MessageArray *) tmp)->messages[0], client->game.playableTopics, topics->nTopics);
        currentOperation->step++;

    case 1:
        return operationCreate(sendMessage, context, socket, tmp);
    
    case 2:
        return OP_OK;

    case 3: // Ricevo la risposta
        return operationCreate(recvMessage, context, socket, &currentOperation->tmp);

    case 4:
        // Ottengo la risposta
        ((MessageArray *) currentOperation->tmp)->messages[0].toFree = true;
        client->game.playing = ntohl( *(int32_t*) ((MessageArray *)currentOperation->tmp)->messages[0].payload);
        messageArrayDestroy((MessageArray**) &currentOperation->tmp);
        
        if (!client_checkTopicIndex(client, topics, client->game.playing)) // Verifico l'indice
            return false;
        
        if (client->game.playing < 0 || !client_setPlayed(client, topics, client->game.playing)) // Aggiorno su disco
        {
            sendCommand(socket, CMD_NOTVALID);
            return OP_FAIL;
            break;
        }

        client->game.currentQuestion = 0;
        if (!client_quizInit(context, socket, topics)) // Inizializzo il gioco, shuffle
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

    if (!message_array)
        return OP_FAIL;

    MessageArray *msgs = (MessageArray *) message_array;

    switch (currentOperation->step) // Incremento non automatico
    {
        case 0: 
            if (sendCommand(socket, CMD_MESSAGE)) // Il client si deve aspettare questo comando
                FD_SET(socket, &context->write_fds);
            else 
                return OP_FAIL;

            client->sending = true; // Da adesso verrà richiamata questa funzione ogni qual volta il socket è ready in scrittura
            currentOperation->count = -1; // Inizializzazione
            currentOperation->step++;

        case 1:
            OperationResult ret;

            bool block = !msgs->isInterruptible;
            while (currentOperation->count < msgs->size) // Invio la dimensionde del MessageArray
            {
                Message * msg;
                if (currentOperation->count == -1)
                    msg = &msgs->messages[msgs->size]; // L'ultimo messaggio contiene il numero di MessageArray
                else
                    msg = &msgs->messages[currentOperation->count]; // Indirizzamento normale

                int lenght_size = sizeof(msg->lenght);
                if (msg->transmitted < lenght_size) // Invio la dimensione del messaggio
                {
                    int lenght = htonl(msg->lenght);
                    if ((ret = sendData(socket, &lenght, lenght_size, &msg->transmitted, block)) != OP_DONE)
                        return ret; // Ritorna in caso di non completamento
                }

                if (msg->payload) // Invio effettivamente il messaggio
                {
                    unsigned int sent = msg->transmitted - lenght_size;

                    if ((ret = sendData(socket, msg->payload, msg->lenght, &sent, block)) != OP_DONE)
                        return ret; // Ritorna in caso di non completamento
                }

                currentOperation->count++;
            }

            currentOperation->step++;
            client->sending = false;
            FD_CLR(socket, &context->write_fds);
            return OP_OK;

        case 2:
            if (recvCommand(socket) == CMD_OK) // Mi aspetto di ricevere conferma
                return OP_DONE;
            else
                return OP_FAIL;

            break;

        default:
            return OP_FAIL;    
    }

    return OP_FAIL;
}

OperationResult recvMessage(ClientsContext *context, int socket, void *pointer)
{
    Client * client = context->clients[socket];
    OperationResult ret = OP_OK;

    Operation *currentOperation = client->operation->data;

    MessageArray ** msgs = (MessageArray **) pointer; // Voglio allocare il MessageArray all'indirizzo che mi è stato passato

    switch (currentOperation->step) // Non incremento automaticamente
    {
    case 0:
        // if(*msgs != NULL) // Parametri limite, non implementato, uso i #define
        //     currentOperation->tmp = *msgs;

        *msgs = messageArray(0);
        currentOperation->count = -1; // Inizializzazione
        currentOperation->step++;
        // continuo
    case 1:
        if (currentOperation->count == -1) // Inizializzazione
        {
            Message *tmp = &(*msgs)->messages[0];
            if ((ret = recvData(socket, &tmp->lenght, sizeof(tmp->lenght), &tmp->transmitted)) == OP_DONE) // Ricevo dimensione del MessageArray
            {
                int size = ntohl(tmp->lenght);
                messageArrayDestroy((MessageArray **) msgs);

                if (size > CLIENT_MAX_MESSAGEARRAY_SIZE) // Limite HARD CODED 
                    return OP_FAIL;

                *msgs = messageArray(size);
                currentOperation->count = 0; // Uso count per indicizzare il messaggio corrente
            }
            return (bool) ret;
        }

        while (currentOperation->count < (*msgs)->size)
        {
            Message * msg = &(*msgs)->messages[currentOperation->count];

            if (msg->transmitted < sizeof(msg->lenght)) // Ricezione della dimensione del messaggio e allocazione
            {
                if ((ret = recvData(socket, &msg->lenght, sizeof(msg->lenght), &msg->transmitted)) == OP_DONE)
                    {
                        msg->lenght = ntohl(msg->lenght);
                        if (msg->lenght > CLIENT_MAX_MESSAGE_LENGHT) // Limite HARD CODED 
                            return OP_FAIL;

                        msg->payload = (void *) malloc(msg->lenght + 1); // Aggiungo un byte in più per il carattere di terminazione nullo
                        if (!msg->payload)
                            return OP_FAIL;

                        msg->toFree = true; // Di default non voglio mantenere la risposta in memoria alla distruzione del MessageArray
                    }
                else
                    return (bool) ret; // Se non ho ricevuto tutto il messaggio ripeto dopo (o fallimento) 
            }

            if (msg->lenght) // Ricezione del messaggio vero e proprio
            {
                unsigned int sent = msg->transmitted - sizeof(msg->lenght);

                if ((ret = recvData(socket, msg->payload, msg->lenght, &sent)) == OP_DONE)
                    *(char*) (msg->payload + msg->lenght) = '\0'; // Proteggo le stringhe ponendo il byte (in più) nullo
                else
                    return (bool) ret; // Se non ho ricevuto tutto il messaggio ripeto dopo (o fallimento) 
            }

            currentOperation->count++; // Messaggio successivo
        }

        currentOperation->step++; // Se arrivo a questo punto ho ricevuto tutto il MessageArray 

    case 2:
        return sendCommand(socket, CMD_OK) ? OP_DONE : OP_FAIL; // Proseguo inviando conferma

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
        if (sendCommand(socket, CMD_NOTVALID))
            return OP_DONE;
        return OP_FAIL;
    }

    Topic *currentTopic = &topics->topics[ client->game.playing ];
    Question *currentQuestion = client->game.questions[ client->game.currentQuestion ];

    switch (currentOperation->step++)
    {
    case 0:
        if (client->game.playing < 0 || client->game.currentQuestion < 0) // Non sta giocando
        {
            if (sendCommand(socket, CMD_NOTVALID))
                return OP_DONE;
            else
                return OP_FAIL;
        }
        else
        {
            if (sendCommand(socket, CMD_OK)) // Confermo e preparo il messaggio
            {
                MessageArray *question_msg = messageArray(1);
                messageString(&question_msg->messages[0], currentQuestion->question, false);
                currentOperation->tmp = question_msg;
            }
            else return OP_FAIL;
        }
        currentOperation->step++;

    case 1:
        // e invio il messaggio
        MessageArray *question_msg = currentOperation->tmp;
        return operationCreate(sendMessage, context, socket, question_msg);
    
    case 2: // The function will be called again after the termination of next operation
        currentOperation->tmp = NULL;
        return OP_OK;
    
    case 3:
        switch( recvCommand(socket) ) // Mi comporto diversamente in base alla conferma
        {
            case CMD_ANSWER:
                currentOperation->step = 5;
                return OP_OK; // Wait for the next message
            
            case CMD_RANK:
                MessageArray * tmp = messageArray(2 * context->scoreboard.nTopics);
                if (!tmp)
                    return OP_FAIL;

                tmp->isInterruptible = false;

                for (int i = 0; i < SCOREBOARD_SIZE * context->scoreboard.nTopics; i++)
                    messageStringReady(&tmp->messages[i], 
                                        context->scoreboard.serialized[i].string, 
                                        context->scoreboard.serialized[i].serialized_lenght,
                                        false);

                operationCreate(sendMessage, context, socket, tmp);
                return OP_OK;

            default:
                return OP_FAIL;
        }
    
    case 4: // Ritorno dalla sendMessage (classifica)
        currentOperation->step = 3; // Ritorno in attesa della risposta
        currentOperation->tmp = NULL;
        return OP_OK;

    case 5:
        return operationCreate(recvMessage, context, socket, &currentOperation->tmp);

    case 6:
        MessageArray *answer_msg = currentOperation->tmp;
        answer_msg->messages[0].toFree = true;
        
        // Verifica della risposta
        if (!stricmpTol(answer_msg->messages[0].payload, 
                        currentQuestion->answer, 
                        MAX_ANSWER_ERRORS,
                        SMALL_ANSWER))
        { // Caso risposta corretta
            if (!sendCommand(socket, CMD_CORRECT))
                return OP_FAIL;
            
            // Incremento il punteggio
            scoreboard_increaseScore(&context->scoreboard, 
                                        client->game.score[client->game.playing], 
                                        client->game.playing);
        }
        else // Caso risposta errata
            if (!sendCommand(socket, CMD_WRONG))
                return OP_FAIL;

        if (++client->game.currentQuestion >= currentTopic->nQuestions) // Domande terminate
        {
            scoreboard_completedScore(&context->scoreboard, client->game.score[client->game.playing],
                            client->game.playing);

            client->game.playing = -1;
            client->game.currentQuestion = -1;
        }

        return OP_DONE;

    default:
        break;
    }

    return OP_FAIL;
}