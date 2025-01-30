#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../shared/commands.h"
#include "../shared/list.h"
#include "../shared/message.h"

#define SEND_MAX_MESSAGEARRAY_SIZE 1
#define SEND_MAX_MESSAGE_LENGHT 256

bool serverTimeout(int, int);

bool sendCommand(int, enum Command);
enum Command recvCommand(int);

bool sendMessage(int socket, MessageArray * msgs);
MessageArray * recvMessage(int);

bool sendData(int socket, void * buffer, unsigned int lenght);
bool recvData(int socket, void * buffer, unsigned int lenght);