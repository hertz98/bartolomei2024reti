#include <stdbool.h>

/// @brief Ottiene la directory padre
/// @param  path La stringa contentente il percorso
/// @return true in caso di modifica
bool parentDirectory(char * path);

/// @brief Rimuove i primi caratteri non alfabetici da una stringa
/// @param string La stringa contentente il nome del file
/// @return true in caso di modifica
bool removeNumbering(char * string);

/// @brief Rimuove l'estensione dal nome del file
/// @param path La stringa contenente il nome del file
/// @return true in caso di modifica
bool removeExtension(char * path);

/// @brief Ottiene il percorso dell'eseguibile del file
/// @param string Stringa in cui scrivere il persorso, se NULL alloca dinamicamente
/// @return Il puntatore alla stringa contenente il percorso dell'eseguibile
char * executablePath(char * string);

/// @brief Sostituisce il carattere di nuova linea '\\n' con quello di termine stringa '\\0'
/// @param path Stringa contenente il carattere di nuova linea'\n' 
/// @return true in caso di modifica
bool newlineReplace(char * path);
