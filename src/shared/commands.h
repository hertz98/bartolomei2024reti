enum Command 
{
    CMD_NONE,
    CMD_ACK, // Aknowledge
    CMD_OK,
    CMD_REGISTER, // Client: Ask register
    CMD_REQMESSAGE, // Request a message
    CMD_MESSAGE, // Ask to send a message
    CMD_SIZE, // Ask for size
    CMD_STRING, // Ask for string
    CMD_RANKING, // Client: ask for ranking
    CMD_STOP = false, // Terminate
    CMD_NOTVALID
};