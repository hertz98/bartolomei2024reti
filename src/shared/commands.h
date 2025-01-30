enum Command 
{
    CMD_STOP = false, // Terminate
    CMD_NONE, 
    CMD_OK, // Confirms positively 
    CMD_REGISTER, // Client: Ask register
    CMD_MESSAGE, // Ask to send a message
    CMD_RANKING, // Client: ask for ranking
    CMD_NOTVALID, // Not valid client name
    CMD_EXISTING, // Name duplicate
    CMD_TOPICS, // Client ask for topics data
    CMD_TOPICPLAY, // Client ask for play a topic
    CMD_NEXTQUESTION, // Client ask to play next question
    CMD_CORRECT, // Server informs that the answer is correct
    CMD_WRONG, // Server informs that the answer is wrong
};