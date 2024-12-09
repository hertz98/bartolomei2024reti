enum Command 
{
    NONE,
    ACK, // Aknowledge
    OK,
    REGISTER, // Client: Ask register
    MESSAGE, // Ask to send a message
    RANKING, // Client: ask for ranking
    STOP, // Terminate
};