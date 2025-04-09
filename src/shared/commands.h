/* COMMANDS.H
 * Elenco dei comandi (bytes) che vengono usati per sincronizzare la logica del server e del client
*/

#pragma once

#ifndef COMMANDS_HEADER
#define COMMANDS_HEADER

typedef enum Command 
{
    CMD_STOP = false, // Chiusura o errore di comunicazione
    CMD_NONE, // Niente o domande terminate
    CMD_OK, // Conferma positiva
    CMD_REGISTER, // Client: Richiesta di registrazione
    CMD_MESSAGE, // Messaggio, epilogo
    CMD_SCOREBOARD, // Client: Richiesta classifica
    CMD_NOTVALID, // Server: Nome client non valido
    CMD_EXISTING, // Server: client duplicato
    CMD_TOPICS, // Client: Richiesta informazioni sui topics
    CMD_TOPICS_PLAYABLE, // Client: Richiesta informazioni sui topics giocabili
    CMD_SELECT, // Client: Selezione topic da giocare
    CMD_ANSWER, // Client: Invio risposta (epilogo)
    CMD_NEXTQUESTION, // Client: Richiesta prossima domanda
    CMD_CORRECT, // Server: Risposta corretta
    CMD_WRONG, // Server: Risposta errata
    CMD_FULL, // Server: server pieno
} Command;

#endif