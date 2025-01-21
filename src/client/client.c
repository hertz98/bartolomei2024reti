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

void clear();
bool mainMenu();
bool signup();
bool topicsSelection();
char * newlineReplace(char * string);
bool readUser_int(int * number);

struct sockaddr_in server_addr;
int sd;

struct Context
{
    char name[255];
    MessageArray * topics;
} context;


void socketclose()
{ 
    close(sd);
    printf("\n CTRIL+C: Socket chiuso\n");
    exit(0);
}

int init(int argc, char ** argv)
{
    if (argc < 3)
    {
        printf("Argomenti insufficienti\n");
        return(1);
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr)); // Pulizia
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = atoi( argv[2] );

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) == -1)
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

    topicsSelection();

    while(1);

    sendCommand(sd, CMD_STOP);

    close(sd);

    return(0);
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
        messageArrayDestroy(context.topics, NULL);

    context.topics = recvMessage(sd);

    if (!context.topics)
        return false;

    if (!context.topics->size)
    {
        printf("Nessun quiz disponibile per l'utente %s\n", context.name);
        printf("Premere [Invio] per terminare\n");
        while(getchar() != '\n');
        exit(0);
    }

    printf("Quiz disponibili\n+++++++++++++++++++++++++++++++\n");

    for (int i = 0; i < context.topics->size; i++)
    {
        context.topics->messages[i].toFree = true;
        printf("%d - %s\n", i + 1, (char*) context.topics->messages[i].data);
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
    messageArrayDestroy(message_sel, NULL);

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