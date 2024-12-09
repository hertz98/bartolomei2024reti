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
    void * tmp;
    // Timeout
    time_t recv_timestamp;
    // Shuffle array
};

bool clientAdd(fd_set *, struct Client *, int);
void clientRemove(fd_set *, struct Client *, int);
bool clientFree(fd_set *, struct Client *);

bool clientTimeout(struct Client *, int);

bool sendMessage(struct Client *, char *);
bool sendCommand(struct Client *, enum Command);
enum Command recvCommand(struct Client *);
bool recvMessage(struct Client *, char *);