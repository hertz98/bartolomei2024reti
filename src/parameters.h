#pragma once

#ifndef PARAM_HEADER
#define PARAM_HEADER

/******************** PARAMETRI GENERALI ********************/

#define DEFAULT_BIND_IP "127.0.0.1"
#define DEFAULT_BIND_PORT 1234

#define MAX_CLIENTs 32
#define REFRESH_RATE 1000

// #define BY_SPECIFICATIONS // Abilitare per l'esame

#ifndef BY_SPECIFICATIONS
    #define DEBUG
    #define PRINT_BY_SPECS
    #define KEEP_SCORE_ON_CLIENT_REMOVE
#endif

#define MAX_ANSWER_ERRORS 1     // Massimo numero di caratteri errati nella risposta
#define SMALL_ANSWER 4      // Non permettere errori su risposte più piccole di SMALL_ANSWER caratteri

/******************** PARAMETRI TOPICS ********************/

// Il nome non può essere più lungo di PATH_MAX - ESTENSIONE
#define CLIENT_NAME_MAX 32
#define CLIENT_NAME_MIN 4

/******************** PARAMETRI TOPICS ********************/

#define DATA_DIR "./data/"
#define TOPICS_DIR "./topics/"
#define USERS_DIR "./users/"

/******************** PARAMETRI OPERATIONS ********************/

#define MAX_OPERATIONS_PER_CLIENT 5

// Limito le dimensioni del messaggi del client in base alle esigenze (hard coded)
#define CLIENT_MAX_MESSAGEARRAY_SIZE 1
#define CLIENT_MAX_MESSAGE_LENGHT 256

/******************** PARAMETRI SCOREBOARD ********************/

#define DEFAULT_SCOREBOARD_ALLOCATION 4096

#endif