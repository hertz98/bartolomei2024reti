/* COMMANDS.H
 * Elenco dei comandi (bytes) che vengono usati per sincronizzare la logica del server e del client
*/

#pragma once

#ifndef COMMANDS_HEADER
#define COMMANDS_HEADER

typedef enum Command 
{
    CMD_STOP = false, // Terminate
    CMD_NONE, // Empty
    CMD_OK, // Confirms positively 
    CMD_REGISTER, // Client: Ask register
    CMD_MESSAGE, // Ask to send a message
    CMD_SCOREBOARD, // Client wants the rank
    CMD_NOTVALID, // Invalid client name or invalid request
    CMD_EXISTING, // Name duplicate
    CMD_TOPICS, // Client ask for topics data
    CMD_TOPICS_PLAYABLE, // Client ask for its playable topics
    CMD_SELECT, // Client ask to select a topic
    CMD_ANSWER, // Client wants to send an answer
    CMD_NEXTQUESTION, // Client ask to play next question
    CMD_CORRECT, // Server informs that the answer is correct
    CMD_WRONG, // Server informs that the answer is wrong
} Command;

#endif