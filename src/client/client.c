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
void topicsSelection();
char * newlineReplace(char * string);

struct sockaddr_in server_addr;
int sd;

char ** topics;
int nTopics;

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
    int ret;

    while(true)
    {
        clear();

        printf("Trivia Quiz\n+++++++++++++++++++++++++++++++\nMenù:\n");
        printf("1 - Comincia una sessione di Trivia\n");
        printf("2 - Esci\n");
        printf("+++++++++++++++++++++++++++++++\nLa tua scelta: ");
        fflush(stdout);

        char buffer[10];
        if ((ret = read(STDIN_FILENO, buffer, sizeof(buffer))) <= 0)
            continue;
        buffer[ret - 1] = '\0';

        if (!strncmp(buffer, "1", 1))
            return true;
        else if (!strncmp(buffer, "2", 1))
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
    char buffer[255];
    printf("Trivia Quiz\n+++++++++++++++++++++++++++++++\n");

    while(true)
    {
        printf("Scegli un nickname (deve essere univoco): ");
        fflush(stdout);

        if ((ret = read(STDIN_FILENO, buffer, sizeof(buffer))) <= 0)
            continue;
        buffer[ret - 1] = '\0';

        if (!sendCommand(sd, CMD_REGISTER))
        {
            printf("Errore nella comunicazione");
            return false;
        }

        if ( recvCommand(sd) == CMD_OK)
        {
            MessageArray *tmp = messageArray(1);
            messageString(&tmp->messages[0], buffer, false);
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

void topicsSelection()
{
    if (!(sendCommand(sd, CMD_TOPICS) && recvCommand(sd) == CMD_OK))
    {
        printf("Errore nella comunicazione");
        exit(1);
    }
    recvCommand(sd);

    printf("Quiz disponibili\n");

    MessageArray * messages = recvMessage(sd);
    for (int i = 0; i < messages->size; i++)
        printf("%s\n", (char*) messages->messages[i].data);

    return;

    // if (!topics)
    // {
    //     char * tmp, *topic;
    //     recvMessage(sd, &tmp);

    //     topics = (char **) malloc(sizeof(char*) * (++nTopics));
    //     topics[0] = tmp;

    //     for (int i = 1; (topic = newlineReplace(tmp)); i++)
    //     {
    //         topics = (char **) realloc(topics, sizeof(char*) * (++nTopics));
    //         topics[i] = topic;
    //     }
    // }

    for (int i = 0; i < nTopics; i++)
        printf("%s\n",topics[i]);
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