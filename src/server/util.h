/* UTIL.H
 * Funzioni di utilità
*/

#pragma once

#ifndef UTIL_HEADER
#define UTIL_HEADER

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

/// @brief Comparazione tra stringhe case Insensitive
/// @param string1
/// @param string2
/// @return Ritorna 0 se sono uguali, >0 se string1 > string2, <0 se string1 < string2
int stricmp(const char * string1, const char * string2);

/// @brief Verifica che la stringa data sia alfanumerica
/// @param string Stringa da verificare
/// @return true se la stringa è alfanumerica
bool isAlphaNumeric(const unsigned char * string);

/// @brief Usa l'algoritmo di fisher-yates per mescolare gli elementi di un array di puntatori
/// @param array Puntatore all'array di puntatori
/// @param arraySize Numero di elementi dell'array
void shuffleArrayPtr(void **array, int arraySize);

/// @brief Come strcpy copia la stringa sorgente nella stringa di destinazione
/// @param dst Stringa destinazione
/// @param src Stringa sorgente
/// @param allocatedSize Puntatore all'intero contenente la dimensione corrente della stringa
/// @param pos Posizione dove iniziare la copia
/// @return The number of bytes copied, excluded the null char
int strcpyResize(char **dst, const char *src, int *allocatedSize, int pos);

/// @brief Verifica se la sottostringa è presente nella stringa data
/// ma verifica che sia una parola, e ammettendo una tolleranza di errori
/// @param string 
/// @param substring parola di cui verificarne la presenza
/// @param tol Numero massimo di errori nella parola
/// @return True nel caso la parola sia contenutra nella stringa, altrimenti false
bool wordInString(const char * string, const char * substring, int tol);

/// @brief Simile a stricmp, ma ammette un numero di errori, le stringhe
/// devono avere lo stesso numero di caratteri, inoltre la tolleranza agli
/// errori non si applica ai numeri e alle risposte brevi
/// @param string1 
/// @param string2 
/// @param tol Numero massimo di errori
/// @param small Numero di caratteri per cui una risposta si considera breve
/// @return Ritorna 0 se le stringhe corrispondono non superando tol errori
int stricmpTol(const char *string1, const char *string2, int tol, int small);

void stringLower(char *string);

#endif