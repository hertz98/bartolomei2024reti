#include "clients.h"

bool clientAdd(Client, int);

void clientRemove(Client, int);
bool clientFree(Client);

bool clientTimeout(Client, int);

bool sendMessage(Client, char *);
bool sendCommand(Client, Command);
enum Command recvCommand(Client);
char * recvMessage(Client);