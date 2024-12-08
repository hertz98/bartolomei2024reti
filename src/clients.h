#include <time.h>
#include <commands.h>

#ifndef MAX_CLIENT_NAME
#define MAX_CLIENT_NAME 32
#endif

enum Status
{
    NONE,
    REGISTERING,
    REGISTERED,
    PLAYING
};

struct ClientStatus
{
    Status current;
    int topic,
        question;
};

struct Client
{
    char name[MAX_CLIENT_NAME];
    int score;
    ClientStatus status;
    time_t recv_timestamp;
};

bool clientAdd(Client, int);
void clientRemove(Client, int);
bool clientFree(Client);

bool clientTimeout(Client, int);

bool sendMessage(Client, char *);
bool sendCommand(Client, Command);
enum Command recvCommand(Client);
char * recvMessage(Client);