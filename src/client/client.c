#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "../parameters.h"
#include "server_comm.h"
#include "../shared/message.h"
#include "../shared/list.h"

/********** DEFINIZIONI **********/

typedef enum InputType {INPUT_INT, INPUT_STRING} InputType;

/********** PROTOTIPI DI FUNZIONE **********/

void clear();
bool mainMenu();
bool signup();
bool topicsSelection();
char * newlineReplace(char * string);
bool playTopic();
void readUser_Enter();
bool getTopicsData();
bool scoreboard();
int input(InputType type, void * output, int size, bool server, bool showscore, bool endquiz);
int init(int argc, char ** argv);

/// @brief Converte l'indice del topic dal punto di vista dell'utente a quello
/// dal punto di vista dell'array dei topic nel server
/// @return Indice dello stesso topic corrispondente nell'array dei topics
int client_playableIndex(int playable);

/********** VARIABILI GLOBALI **********/

struct sockaddr_in server_addr;
int sd;

struct Context
{
    char name[255];
    int nTopics;
    char ** topics;
    bool * playable;
    int playing;
} context;

/********** METODI **********/

int main (int argc, char ** argv)
{
    if (!mainMenu())
        return 0;
    
    if (!init(argc, argv))
        return 1;

    if (!signup())
        return 2;

    if (!getTopicsData())
        return 3;

    while(true)
    {
        if (!topicsSelection() || !playTopic())
            break;
    }

    close(sd);

    return 4;
}

int init(int argc, char ** argv)
{
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1)
    {
        perror("Errore nella creazione del socket:");
        return false;
    }
    
    memset(&server_addr, 0, sizeof(server_addr)); // Pulizia
    server_addr.sin_family = AF_INET;

    char * addr;
    switch (argc)
    {
    case 0:
    case 1:
        addr = DEFAULT_BIND_IP;
        server_addr.sin_port = htons( DEFAULT_BIND_PORT );
        break;
    case 2:
        addr = DEFAULT_BIND_IP;
        server_addr.sin_port = htons( atoi( argv[1] ));
        break;
    case 3:
    default:
        addr = argv[1];
        server_addr.sin_port = htons( atoi( argv[2] ));
        break;
    }

    if (inet_pton(AF_INET, addr, &server_addr.sin_addr) == -1)
    {
        printf("Errore nella conversione dell'indirizzo ip\n");
        return false;
    }

    if (connect(sd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
    {
        perror("Errore nella connect");
        return false;
    }

    memset(&context, 0, sizeof(context));

    return true;
}

bool mainMenu()
{
    clear();
    printf("Trivia Quiz\n+++++++++++++++++++++++++++++++\nMenù:\n");
    printf("1 - Comincia una sessione di Trivia\n");
    printf("2 - Esci\n");
    printf("+++++++++++++++++++++++++++++++\n");

    while(true)
    {
        printf("La tua scelta: ");
        fflush(stdout);

        int selection, ret;
        if ((ret = input(INPUT_INT, &selection, sizeof(selection), false, false, false)) <= 0)
            continue;

        if (selection == 1)
            return true;
        else if (selection == 2)
            return false;
    }

}

inline void clear()
{
    printf("\033[H\033[J"); // or system("clear");
}

bool signup()
{
    int ret;

    clear();
    printf("Trivia Quiz\n+++++++++++++++++++++++++++++++\n");

    while(true)
    {
        printf("Scegli un nickname (deve essere univoco): ");
        fflush(stdout);

        if ((ret = input(INPUT_STRING, context.name, sizeof(context.name), true, false, false)) <= 0)
            continue;

        if (!sendCommand(sd, CMD_REGISTER))
        {
            printf("Errore nella comunicazione\n");
            return false;
        }

        if ( recvCommand(sd) != CMD_OK)
        {
            printf("Rifiutato dal server\n");
            return false;
        }

        MessageArray *tmp = messageArray(1);
        messageString(&tmp->messages[0], context.name, false);
        sendMessage(sd, tmp);

        switch(recvCommand(sd))
        {
            case CMD_OK:
                return true;
            
            case CMD_EXISTING:
                printf("Nome duplicato!\n\n");
                break;

            case CMD_NOTVALID:
                printf("Nome non valido\n\n");
                break;

            case false:
                printf("Rifiutato dal server\n");
                return false;

            default:
                printf("Errore nella comunicazione\n");
                return false;
        }
    }
    return false;
}

bool getTopicsData()
{
    if (sendCommand(sd, CMD_TOPICS) && recvCommand(sd) != CMD_OK)
    {
        printf("Errore nello scaricamento dei topics\n");
        return false;
    }

    if (context.topics)
    {
        for (int i = 0; i < context.nTopics; i++)
            free(context.topics[i]);
        free(context.topics);
    }

    MessageArray *tmp = recvMessage(sd);
    if (!tmp)
    {
        printf("Errore nella ricezione dei dati sui topics dal server\n");
        return false;
    }

    context.nTopics = tmp->size - 1;
    context.topics = messageArray2StringArray(tmp);
    context.playable = (bool*) tmp->messages[context.nTopics].payload;
    messageArrayDestroy(&tmp);

    if (!context.nTopics)
    {
        printf("Nessun quiz disponibile nel server\n");
        readUser_Enter();
        exit(EXIT_SUCCESS);
    }

    return true;
}

int client_playableIndex(int playable)
{
    if (playable < 0 || playable >= context.nTopics)
        return -1;
    
    for (int i = 0, n = 0; i < context.nTopics; i++)
        if (context.playable[i])
            if (n++ == playable)
                    return i;

    return -1;
}

bool recvPlayables()
{
    if (!(sendCommand(sd, CMD_TOPICS_PLAYABLE) && recvCommand(sd) == CMD_OK))
    {
        printf("Errore nella comunicazione");
        return false;
    }

    if (context.playable)
    {
        free(context.playable);
        context.playable = NULL;
    }

    MessageArray * tmp = recvMessage(sd);
    context.playable = tmp->messages[0].payload;

    return true;
}

bool topicsSelection()
{
    if (!context.playable && !recvPlayables())
    {
        printf("Problema di comunicazione con il server\n");
        return false;
    }

    int nPlayable = 0;
    for (int i = 0; i < context.nTopics; i++)
        if (context.playable[i])
            nPlayable++;

    if (!nPlayable)
    {
        clear();
        printf("Nessun quiz disponibile per l'utente %s\n", context.name);
        readUser_Enter();
        exit(0);
    }
    
    while(true)
    {
        clear();

        printf("Quiz disponibili\n+++++++++++++++++++++++++++++++\n");

        for (int i = 0, n = 0; i < context.nTopics; i++)
            if (context.playable[i])
                printf("%d - %s\n", ++n, (char*) context.topics[i]);

        printf("+++++++++++++++++++++++++++++++\n");

        printf("La tua scelta: ");
        fflush(stdout);

        int ret;
        do
        {
            printf("Risposta: ");
            fflush(stdout);
        } while ((ret = input(INPUT_INT, &context.playing, sizeof(context.playing), true, true, true)) == 0);
        
        if (ret > 0 &&  context.playing >= 1 && context.playing <= nPlayable)
            break;
    }

    if (!(sendCommand(sd, CMD_SELECT) && recvCommand(sd) == CMD_OK))
    {
        printf("Errore nella comunicazione");
        return false;
    }

    context.playing = client_playableIndex(context.playing - 1);

    context.playable[context.playing] = false;

    MessageArray * message_sel = messageArray(1);
    messageInteger(&message_sel->messages[0], context.playing);
    sendMessage(sd, message_sel);

    messageArrayDestroy(&message_sel);

    return true;
}

char * newlineReplace(char * string)
{
    for( ; *string != '\0'; string++)
    {
        if(*string == '\n')
        {
            *string = '\0';
            return string + 1;
        }
    }
    return NULL;
}

void readUser_Enter()
{
    printf("Premere [Invio] per continuare...\n");
    while(getchar() != '\n');
}

bool scoreboard()
{
    clear();

    if (!sendCommand(sd, CMD_RANK) || recvCommand(sd) != CMD_OK)
    {
        printf("Errore di comunicazione con il server\n");
        return false;
    }

    MessageArray * tmp = recvMessage(sd);

    if (!tmp)
    {
        printf("Qualcosa è andato storto nella ricezione della classifica\n");
        return false;
    }

    for (int i = 0; i < tmp->size; i++)
        printf("%s\n", (char *) tmp->messages[i].payload);

    messageArrayDestroy(&tmp);

    return true;
}

bool playTopic()
{
    while(true) // Exyernal topic playing loop
    {
        if (!sendCommand(sd, CMD_NEXTQUESTION))
        {
            printf("Erroe di comunicazione");
            exit(0);
        }
        
        switch (recvCommand(sd))
        {
        case CMD_OK:
            break;

        case CMD_NONE:
            return true;
        
        default:
            printf("Errore di comunicazione con il server\n");
            exit(EXIT_FAILURE);
        }

        MessageArray *question_msg = recvMessage(sd);
        question_msg->messages[0].toFree = true;

        char buffer[CLIENT_MAX_MESSAGE_LENGHT];
        while(true) // Printing loop
        {
            clear();
            printf("Quiz - %s\n+++++++++++++++++++++++++++++++\n",
                (char*) context.topics[context.playing]);

            printf("%s\n", (char *) question_msg->messages[0].payload);
            printf("\n");

            int ret;
            do
            {
                printf("Risposta: ");
                fflush(stdout);
            } while ((ret = input(INPUT_STRING, buffer, sizeof(buffer), true, true, true)) == 0);

            if (ret > 0)
                break;
        }

        if ( !sendCommand(sd, CMD_ANSWER) )
        {
            printf("Errore nell'invio della risposta\n");
            exit(EXIT_FAILURE);
        }

        MessageArray *answer_msg = messageArray(1);
        messageString(&answer_msg->messages[0], buffer, false);
        if (!sendMessage(sd, answer_msg))
        {
            printf("Errore nell'invio della risposta\n");
            return false;
        }

        messageArrayDestroy(&question_msg);
        messageArrayDestroy(&answer_msg);

        switch (recvCommand(sd))
        {
        case CMD_CORRECT:
            printf("Risposta corretta\n");
            break;

        case CMD_WRONG:
            printf("Rispsota errata\n");
            break;
        
        default:
            printf("Errore nella ricezione del verdetto\n");
            return false;
            break;
        }

    readUser_Enter();
    }
}

int input(InputType type, void * output, int size, bool server, bool showscore, bool endquiz)
{
    char buffer[CLIENT_MAX_MESSAGE_LENGHT];
    int ret;

    while(server)
    {
        fd_set test_fds;
        FD_ZERO(&test_fds);
        FD_SET(sd, &test_fds);
        FD_SET(STDIN_FILENO, &test_fds);

        if (select(sd + 1, &test_fds, NULL, NULL, NULL) == -1)
        {
            perror("Errore: ");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(sd, &test_fds))
            if (recvCommand(sd) == false)
            {
                printf("Connessione con il server interrotta\n");
                exit(EXIT_FAILURE);
            }
        
        if (FD_ISSET(STDIN_FILENO, &test_fds))
            break;
        
    }

    if ((ret = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[ret - 1] = '\0';

        if (!strncmp(buffer, "quit", sizeof(buffer)) ||
                !strncmp(buffer, "exit", sizeof(buffer)) )
            exit(EXIT_SUCCESS);

        if (showscore && !strncmp(buffer, "show score", sizeof(buffer)))
        {
            if (!scoreboard())
                return false;
            readUser_Enter();
            return -1;
        }

        if (endquiz && !strncmp(buffer, "endquiz", sizeof(buffer)))
        {
            sendCommand(sd, CMD_STOP);
            exit(EXIT_SUCCESS);
        }

        if (type == INPUT_STRING)
        {
            strncpy(output, buffer, size);
            return ret - 1;
        }

        if (type == INPUT_INT && (ret = sscanf(buffer, "%d", (int*) output)) > 0)
            return ret;
        else
            return -1;
    }
    else if (ret == 0)
    {
        printf("EOF rilevato, uscita...\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        perror("Errore: ");
        exit(EXIT_FAILURE);
    }
}