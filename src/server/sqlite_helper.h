#include "sqlite-amalgamation/sqlite3.h"
#include <stdbool.h>

/// @brief Crea il database se necessario e apre la connessione
/// @param connection Connessione al database
/// @param path Percorso del database
/// @return true se va tutto lisco
bool db_init(sqlite3 * connection, char * path);

/// @brief Funzione ad-hoc per creare la connessione
/// @param connection Connessione al database sqlite
/// @param path Percorso del database
/// @return true se la creazione è avvenuta
bool db_create(sqlite3 * connection, char * path);

/// @brief Ritorna se il topic è stato giocato dal giocatore
/// @param connection Connessione al database sqlite già aperta
/// @param player stringa contenente il nome del giocatore (senza numerazione)
/// @param topic stringa contenente il nome del topic (senza numerazione)
/// @return true se il giocatore ha già giocato il topic
bool db_isPlayed(sqlite3 * connection, char * player, char * topic);

/// @brief Registra il topic specificato come giocato al giocatore specificato
/// @param connection Connessione al database sqlite già aperta
/// @param player stringa contenente il nome del giocatore (senza numerazione)
/// @param topic stringa contenente il nome del topic (senza numerazione)
/// @param score si può salvare il risultato
/// @return true se la modifica ha successo
bool db_setPlayed(sqlite3 *connection, char * player, char * topic, int score);

/// @brief Chiude la connessione col database
/// @param connection Connessione al database sqlite già aperta
void db_close(sqlite3 * connection);