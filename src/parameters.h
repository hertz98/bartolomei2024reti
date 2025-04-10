#pragma once

#ifndef PARAM_HEADER
#define PARAM_HEADER

/******************** PARAMETRI GENERALI ********************/

#define DEFAULT_BIND_IP "127.0.0.1"
#define DEFAULT_BIND_PORT 1234

#define MAX_CLIENTs 32
#define MAX_UNREGISTERED 10
#define REFRESH_RATE 1000
#define CLIENTs_ARRAY_INCREMENT 10

// CONTROLLO RISPOSTE
#define MAX_ANSWER_ERRORS 1     // Massimo numero di caratteri errati nella risposta
#define SMALL_ANSWER 4      // Non permettere errori su risposte più piccole di SMALL_ANSWER caratteri

/******************** PARAMETRI CUSTOM ********************/

#define DEBUG
#define CUSTOM_PRINT
//#define KEEP_SCORE_ON_CLIENT_REMOVE
#define RELOAD_SCORES

/******************** PARAMETRI TOPICS ********************/

// Il nome può esserea al più lungo PATH_MAX - ESTENSIONE
#define CLIENT_NAME_MAX 32
#define CLIENT_NAME_MIN 4

/******************** PARAMETRI TOPICS ********************/

#define DATA_DIR "./data/"
#define TOPICS_DIR "./topics/"
#define USERS_DIR "./users/"

/******************** PARAMETRI OPERATIONS ********************/

#define MAX_OPERATIONS_PER_CLIENT 5 // stackable

// Limito le dimensioni del messaggi del client in base alle esigenze (hard coded)
#define CLIENT_MAX_MESSAGEARRAY_SIZE 1
#define CLIENT_MAX_MESSAGE_LENGHT 255

/******************** PARAMETRI SCOREBOARD ********************/

#define DEFAULT_SCOREBOARD_SERIALIZE_ALLOCATION 4096

/******************** ALTRO ********************/

#define COMMAND_SEND_TIMEOUT 2 // (secondi)

// Massimo tempo consecutivo in cui il socket non pronto durante
// l'invio in modo bloccante può bloccare il server prima di essere rimosso
#define MAX_SEND_STALL 2 // (secondi)

#endif