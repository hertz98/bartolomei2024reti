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

#include "server_comm.h"
#include "../shared/message.h"
#include "../shared/list.h"

/********** PARAMETRI **********/

#define DEFAULT_BIND_IP 127.0.0.1
#define DEFAULT_BIND_PORT 1234

/********** PROTOTIPI DI FUNZIONE **********/

void clear();
bool mainMenu();
bool signup();
bool topicsSelection();
char * newlineReplace(char * string);
bool readUser_int(int * number);
bool playTopic();
void readUser_Enter();

/********** VARIABILI GLOBALI **********/

struct sockaddr_in server_addr;
int sd;

struct Context
{
    char name[255];
    MessageArray * topics;
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
        addr = "DEFAULT_BIND_IP";
        server_addr.sin_port = htons( DEFAULT_BIND_PORT );
        break;
    case 2:
        addr = "DEFAULT_BIND_IP";
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

    context.topics = NULL;

    return 0;
}

int main (int argc, char ** argv)
{
    int ret;

    if ((ret = init(argc, argv)))
        return ret;

    if (!mainMenu())
        return 0;

    if (!signup())
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
    enum Command cmd = CMD_STOP;
    
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
            printf("Errore nella comunicazione");
            return false;
        }

        if ( recvCommand(sd) == CMD_OK)
        {
            MessageArray *tmp = messageArray(1);
            messageString(&tmp->messages[0], context.name, false);
            sendMessage(sd, tmp);

            if ((cmd = recvCommand(sd)) == CMD_OK)
            {
                return true;
            }
            else if (cmd == CMD_EXISTING)
            {
                printf("Nome già utilizzato!\n\n");
                continue;
            }
            else if (cmd == CMD_NOTVALID)
            {
                printf("Nome non valido!\n\n");
                continue;
            }
            else
            {
                printf("Rifiutato dal server!");
                return false;
            }
        }
        else
        {
            printf("Rifiutato dal server!");
            return false;
        }
    }
    return false;
}

bool topicsSelection() // TODO: attenersi alle specifiche
{
    clear();

    if (!(sendCommand(sd, CMD_TOPICS) && recvCommand(sd) == CMD_OK))
    {
        printf("Errore nella comunicazione");
        exit(1);
    }

    if (context.topics)
        messageArrayDestroy(&context.topics);

    context.topics = recvMessage(sd);

    if (!context.topics)
        return false;

    if (!context.topics->size)
    {
        printf("Nessun quiz disponibile per l'utente %s\n", context.name);
        readUser_Enter();
        exit(0);
    }

    printf("Quiz disponibili\n+++++++++++++++++++++++++++++++\n");

    for (int i = 0; i < context.topics->size; i++)
    {
        context.topics->messages[i].toFree = true;
        printf("%d - %s\n", i + 1, (char*) context.topics->messages[i].payload);
    }

    printf("+++++++++++++++++++++++++++++++\n");

    int selection;
    while(true)
    {
        printf("La tua scelta: ");
        fflush(stdout);
        
        if (readUser_int(&selection) && selection >= 1 && selection <= context.topics->size)
            break;
    }

    MessageArray * message_sel = messageArray(1);
    messageInteger(&message_sel->messages[0], selection - 1);
    sendMessage(sd, message_sel);

    //messageArrayDestroy(topics, NULL);
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

bool playTopic()
{
    while(true)
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

        clear();
        printf("Quiz - %s\n+++++++++++++++++++++++++++++++\n",
             (char*) context.topics->messages[context.playing].payload);
        
        MessageArray *question_msg = recvMessage(sd);
        question_msg->messages[0].toFree = true;

        printf(question_msg->messages[0].payload);
        printf("\n");

        char buffer[128];
        while(true)
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
        
        MessageArray *answer_msg = messageArray(1);
        messageString(&answer_msg->messages[0], buffer, false);
        sendMessage(sd, answer_msg);

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