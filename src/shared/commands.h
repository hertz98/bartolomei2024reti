enum Command 
{
    NONE,
    ACK, // Aknowledge
    REGISTER, // Client: Ask register
    MESSAGE, // Ask to send a message
    RANKING, // Client: ask for ranking
    STOP, // Terminate
};

enum Status 
{
    NONE,
    REGISTERING,
    SENDING,
    RECEIVING,
};