#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"

bool serverTimeout(int, int);

bool sendCommand(int, enum Command);
enum Command recvCommand(int);

bool sendInteger(int, int);
int recvInteger(int);

bool sendMessage(int, void *);
bool sendString(int, char *, int);

bool recvMessage(int, void **);
bool recvString(int , char **, int);