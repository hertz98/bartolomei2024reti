#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"
#include "../shared/list.h"
#include "../shared/message.h"

bool serverTimeout(int, int);

bool sendCommand(int, enum Command);
enum Command recvCommand(int);

bool sendInteger(int, int);
int recvInteger(int);

bool sendMessage(int socket, MessageArray * msgs);
bool sendString(int, char *, int);

MessageArray * recvMessage(int);
bool recvString(int , char **, int);

bool sendData(int socket, void * buffer, unsigned int lenght);