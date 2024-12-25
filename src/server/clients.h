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
    
    // Shuffle array
} Client;

typedef struct ClientsContext {
    int nClients; // Numero di clients attuali
    int maxClients; // Numero massimo di clients servibili

    fd_set master;
    int fd_max; // Massimo intero nel set master

    Client * clients;
    int allocated; // Numero di strutture allocate per i clients
} ClientsContext;

int clientsInit(ClientsContext ** clientsContext, int max);

bool clientAdd(ClientsContext * context, int socket);
void clientRemove(ClientsContext * context, int socket);
void clientsFree(ClientsContext *context, int socket);

bool sendCommand(Client *, enum Command);
enum Command recvCommand(Client *);

bool sendInteger(Client *, int);
int recvInteger(Client *);

OperationStatus sendMessage(Client *, void *, bool);
bool sendString(Client *, char *, int);

OperationStatus recvMessageProcedure(Client *, void *, bool);
bool recvString(Client *, char **, int);

OperationStatus regPlayer(Client *, void *, bool);
bool nameValid(ClientsContext *, char *);