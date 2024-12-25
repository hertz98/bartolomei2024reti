#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"

typedef enum OperationStatus {
    OP_FAIL = false,
    OP_OK = true,
    OP_DONE
} OperationStatus;

typedef struct Client Client;
typedef struct ClientsContext ClientsContext;

struct Client
{
    int socket; // ridondante
    bool registered;
    char * name;
    int score;

    // Status
    OperationStatus (* operation)(ClientsContext *, int, void *, bool);
    int step;

    int tmp_i;
    void * tmp_p;
    
    // Shuffle array
};

struct ClientsContext {
    int nClients; // Numero di clients attuali
    int maxClients; // Numero massimo di clients servibili

    fd_set master;
    int fd_max; // Massimo intero nel set master

    Client ** clients;
    int allocated; // Numero di strutture allocate per i clients
};

int clientsInit(ClientsContext ** clientsContext, int max);

bool clientAdd(ClientsContext * context, int socket);
void clientRemove(ClientsContext * context, int socket);
void clientsFree(ClientsContext *context, int socket);

bool sendCommand(Client *, enum Command);
enum Command recvCommand(Client *);

bool sendInteger(Client *, int);
int recvInteger(Client *);

OperationStatus sendMessage(ClientsContext *context, int socket, void * buffer, bool init);
bool sendString(Client *, char *, int);

OperationStatus recvMessage(ClientsContext *context, int socket, void * buffer, bool init);
bool recvString(Client *, char **, int);

OperationStatus regPlayer(ClientsContext *context, int socket, void *, bool init);
bool nameValid(ClientsContext * context, int socket, char * name);