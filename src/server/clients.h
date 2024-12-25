#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"

typedef enum OperationStatus {
    OP_FAIL = false,
    OP_OK = true,
    OP_DONE
} OperationStatus;

typedef struct Client
{
    int socket; // ridondante
    bool registered;
    char * name;
    int score;

    // Status
    OperationStatus (* operation)(struct Client *, void *, bool);
    int step;

    int tmp_i;
    void * tmp_p;
    
    // Timeout
    time_t recv_timestamp;
    // Shuffle array
} Client;

bool clientAdd(fd_set *, Client **, int);
void clientRemove(fd_set *, Client **, int);
void clientFree(fd_set *, Client **, int);

bool clientTimeout(Client *, int);

bool sendCommand(Client *, enum Command);
enum Command recvCommand(Client *);

bool sendInteger(Client *, int);
int recvInteger(Client *);

OperationStatus sendMessage(Client *, void *, bool);
bool sendString(Client *, char *, int);

OperationStatus recvMessageProcedure(Client *, void *, bool);
bool recvString(Client *, char **, int);

OperationStatus regPlayer(Client *, void *, bool);
bool nameValid(Client **, char *);