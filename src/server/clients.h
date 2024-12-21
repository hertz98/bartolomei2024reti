#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"

enum OperationStatus {
    OP_FAIL = false,
    OP_OK = true,
    OP_DONE
};

struct Client
{
    int socket; // ridondante
    bool registered;
    char * name;
    int score;

    // Status
    enum OperationStatus (* operation)(struct Client *, void *, bool);
    int step;

    int tmp_i;
    void * tmp_p;
    
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

enum OperationStatus sendMessage(struct Client *, void *, bool);
bool sendString(struct Client *, char *, int);

enum OperationStatus recvMessageProcedure(struct Client *, void *, bool);
bool recvString(struct Client *, char **, int);

enum OperationStatus regPlayer(struct Client *, void *, bool);
bool nameValid(struct Client **, char *);