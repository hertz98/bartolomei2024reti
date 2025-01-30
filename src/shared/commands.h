enum Command 
{
    CMD_STOP = false, // Terminate
    CMD_NONE,
    CMD_OK,
    CMD_REGISTER, // Client: Ask register
    CMD_MESSAGE, // Ask to send a message
    CMD_RANKING, // Client: ask for ranking
    CMD_NOTVALID,
    CMD_EXISTING,
    CMD_TOPICS,
    CMD_NEXTQUESTION,
    CMD_CORRECT,
    CMD_WRONG,
};