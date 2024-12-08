#ifndef MAX_CLIENT_NAME
#define MAX_CLIENT_NAME 32
#endif

#include <time.h>
#include <stdbool.h>
#include "../shared/commands.h"

struct Client
{
    char name[MAX_CLIENT_NAME];
    int score;
    enum Status status;
    void *tmp;
    time_t recv_timestamp;
};

bool clientAdd(struct Client, int);
void clientRemove(struct Client, int);
bool clientFree(struct Client);

bool clientTimeout(struct Client, int);

bool sendMessage(struct Client, char *);
bool sendCommand(struct Client, enum Command);
enum Command recvCommand(struct Client);
char * recvMessage(struct Client);