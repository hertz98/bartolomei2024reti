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

#include "../shared/commands.h"

struct sockaddr_in server_addr;
int sd;

#define BUFFER_SIZE 20
char buffer[BUFFER_SIZE + 1];

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

    buffer[BUFFER_SIZE] = '\0';

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

    strncpy(buffer, "REGISTER", BUFFER_SIZE);

    if (( ret = send(sd, buffer, BUFFER_SIZE, 0)) < 0)
    {
        perror("Errore nella sendto");
        return(1);
    }
    else
        printf("send effettuata %d bytes\n", ret);

    //int addrlen = sizeof(server_addr);
    if (( ret = recv(sd, buffer, BUFFER_SIZE, 0) == -1))
    {
        perror("Errore nella recv");
        return(1);
    }
    if (!strncmp(buffer, "REGISTERED", BUFFER_SIZE))
    {
        printf("Registrato al server\n");
    }
    else
    {
        printf("La registrazione NON ha avuto successo\n");
        return(1);
    }

    while(1)
    {
        //int addrlen = sizeof(server_addr);
        if (recv(sd, buffer, BUFFER_SIZE, 0) == -1)
            printf("Errore nella ricezione\n");
        printf("\n%s\n", buffer);
    }

    return(0);

}