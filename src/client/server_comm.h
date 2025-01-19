#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"
#include "../shared/list.h"
#include "../shared/message.h"

bool serverTimeout(int, int);

bool sendCommand(int, enum Command);
enum Command recvCommand(int);

bool sendMessage(int socket, MessageArray * msgs);
MessageArray * recvMessage(int);

bool sendData(int socket, void * buffer, unsigned int lenght);