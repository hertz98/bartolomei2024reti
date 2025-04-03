/* SERVER_COMM.H
 * Contiene funzioni e strutture dati per la comunicazione con il server
*/

#include <time.h>
#include <stdbool.h>
#include <sys/select.h>

#include "../parameters.h"
#include "../shared/commands.h"
#include "../shared/message.h"

/// @brief Invia un comando al server
/// @param sd Socket del server 
/// @param cmd Comando da inviare
/// @return true in caso di successo, false altrimenti
bool sendCommand(int sd, enum Command cmd);

/// @brief Riceve un comando dal server
/// @param sd Socket del server
/// @return Il comando ricevuto dal server oppure false
Command recvCommand(int sd);

/// @brief Invia un messaggio al server
/// @param socket Socket del server
/// @param msgs MessageArray contentente i messaggi
/// @return true in caso di successo, false altrimenti
bool sendMessage(int socket, MessageArray * msgs);

/// @brief Riceve un messaggio dal server
/// @param sd Socket del server
/// @return Il messageArray dinamicamente allocato o NULL
MessageArray * recvMessage(int sd);

/// @brief Funzione per inviare dati grezzi al server
/// @param socket Socket del server
/// @param buffer Array di bytes contenente i dati grezzi
/// @param lenght Dimensione dell'array
/// @return true in caso di successo, false altrimenti
bool sendData(int socket, void * buffer, unsigned int lenght);

/// @brief Funzione per ricevere dati grezzi al server
/// @param socket Socket del server
/// @param buffer Array di bytes vuoto
/// @param lenght Dimensione dell'array e numero di byte aspettati dal server
/// @return true in caso di successo, false altrimenti
bool recvData(int socket, void * buffer, unsigned int lenght);

/// @brief Verifica se un socket ha ulteriori dati disponibili
/// @param socket Indice del descrittore del socket
/// @return true se il socket ha dati disponibili, false altrimenti (o in caso di errore)
bool client_socketReady(int socket);