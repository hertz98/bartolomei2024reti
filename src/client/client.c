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

/********** PROTOTIPI DI FUNZIONE **********/

void clear();
bool mainMenu();
bool signup();
bool topicsSelection();
char * newlineReplace(char * string);
bool readUser_int(int * number);
bool playTopic();
void readUser_Enter();
bool getTopicsData();
void scoreboard();

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

void socketclose()
{ 
    close(sd);
    printf("\n CTRIL+C: Socket chiuso\n");
    exit(0);
}

int init(int argc, char ** argv)
{
    sd = socket(AF_INET, SOCK_STREAM, 0);
    
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
        return 1;
    }

    if (connect(sd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
    {
        perror("Errore nella connect");
        return 1;
    }

    signal(SIGINT, socketclose);

    memset(&context, 0, sizeof(context));

    return 0;
}

int main (int argc, char ** argv)
{
    int ret;

    if (!mainMenu())
        return 0;
    
    if ((ret = init(argc, argv)))
        return ret;

    if (!signup() || !getTopicsData())
        return 1;

    while(true)
    {
        if (!topicsSelection() || !playTopic())
            break;
    }

    close(sd);

    return(EXIT_FAILURE);
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

        int selection;
        if (!readUser_int(&selection))
            continue;

        if (selection == 1)
            return true;
        else if (selection == 2)
            return false;
    }

}

inline void clear()
{
    printf("\033[H\033[J"); // system("clear");
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

        if ((ret = read(STDIN_FILENO, context.name, sizeof(context.name))) <= 0)
            continue;
        context.name[ret - 1] = '\0';

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
        exit(1);
    }

    context.nTopics = tmp->size;
    context.topics = messageArray2StringArray(tmp);
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

bool topicsSelection() // TODO: attenersi alle specifiche
{
    clear();

    if (!(sendCommand(sd, CMD_TOPICPLAY) && recvCommand(sd) == CMD_OK))
    {
        printf("Errore nella comunicazione");
        exit(1);
    }

    if (context.playable)
    {
        free(context.playable);
        context.playable = NULL;
    }

    MessageArray * tmp = recvMessage(sd);
    context.playable = tmp->messages[0].payload;

    int nPlayable = 0;
    for (int i = 0; i < context.nTopics; i++)
        if (context.playable[i])
            nPlayable++;

    if (!context.playable)
    {
        printf("Nessun quiz disponibile per l'utente %s\n", context.name);
        readUser_Enter();
        exit(0);
    }

    printf("Quiz disponibili\n+++++++++++++++++++++++++++++++\n");

    for (int i = 0, n = 0; i < context.nTopics; i++)
        if (context.playable[i])
            printf("%d - %s\n", ++n, (char*) context.topics[i]);

    printf("+++++++++++++++++++++++++++++++\n");

    while(true)
    {
        printf("La tua scelta: ");
        fflush(stdout);
        
        if (readUser_int(&context.playing) 
            && context.playing >= 1 
            && context.playing <= nPlayable)
            break;
    }
    context.playing = client_playableIndex(context.playing - 1);

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

bool readUser_int(int * number)
{
    char buffer[32];
    if (fgets(buffer, sizeof(buffer), stdin)) 
    {
        if (!strncmp(buffer, "quit", sizeof(buffer)) ||
             !strncmp(buffer, "exit", sizeof(buffer)) )
            exit(0);
        if (sscanf(buffer, "%d", number) > 0)
            return true;
        else
            return false;
    }
    else
    {
        if (feof(stdin))
        {
            printf("EOF rilevato, uscita...\n");
            exit(0);
        }
        return false;
    }
}

void readUser_Enter()
{
    printf("Premere [Invio] per continuare...\n");
    while(getchar() != '\n');
}

void scoreboard()
{
    clear();

    MessageArray * tmp = recvMessage(sd);

    if (!tmp)
    {
        printf("Qualcosa è andato storto nella ricezione della classifica\n");
        exit(1);
    }

    for (int i = 0; i < tmp->size; i++)
        printf("%s\n", (char *) tmp->messages[i].payload);

    messageArrayDestroy(&tmp);
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
            return false;
            break;
        }

        MessageArray *question_msg = recvMessage(sd);
        question_msg->messages[0].toFree = true;

        char buffer[128];
        while(true) // Printing loop
        {
            clear();
            printf("Quiz - %s\n+++++++++++++++++++++++++++++++\n",
                (char*) context.topics[context.playing]);

            printf("%s\n", (char *) question_msg->messages[0].payload);
            printf("\n");

            while(true) // User input loop
            {
                printf("Risposta: ");
                fflush(stdout);
                int ret;
                if ((ret = read(STDIN_FILENO, buffer, sizeof(buffer))) > 1)
                {
                    buffer[ret - 1] = '\0';
                    break;
                }
            }

            if (!strcmp(buffer, "endquiz"))
            {
                sendCommand(sd, CMD_STOP);
                exit(0);
            }
            else if (!strcmp(buffer, "show score"))
            {
                sendCommand(sd, CMD_RANK);
                scoreboard();
                readUser_Enter();
                continue;
            }
            else // It's an answer!
            {
                if ( !sendCommand(sd, CMD_ANSWER) )
                {
                    printf("Errore nell'invio della risposta\n");
                    return false;
                }
                else
                    break; // Internal user input loop
            }
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


