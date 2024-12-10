#ifndef MAX_CLIENT_NAME
#define MAX_CLIENT_NAME 32
#endif

#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"

struct Client
{
    int socket; // ridondante
    char name[MAX_CLIENT_NAME];
    int score;

    // Status
    bool (* operation)(struct Client *, void *);
    int step;

    union {
        int tmp_i;
        void * tmp_p;
    };
    
    // Timeout
    time_t recv_timestamp;
    // Shuffle array
};

bool clientAdd(fd_set *, struct Client **, int);
void clientRemove(fd_set *, struct Client **, int);
void clientFree(fd_set *, struct Client **, int);

bool clientTimeout(struct Client *, int);

bool sendCommand(struct Client *, enum Command);
enum Command recvCommand(struct Client *);

bool sendInteger(struct Client *, int);
int recvInteger(struct Client *);

bool sendMessage(struct Client *, void *);
bool sendMessageProcedure(struct Client *);
bool sendString(int, char *, int);

bool recvMessage(struct Client *, void *);
bool recvMessageProcedure(struct Client *);
bool recvString(int, char *, int);