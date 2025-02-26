#pragma once
#include <ctype.h>
#include <stdbool.h>

/********** LISTA A DOPPIO PUNTATORE **********/

typedef struct DNode {
    void *data; // Puntatore a qualsiasi tipo
    struct DNode *next;
    struct DNode *prev;
} DNode;

/// @brief Alloca un nuovo nodo
/// @param data puntatore al dato che si vuole allocare 
/// @return Il nodo appena allocato, altrimenti NULL
DNode *listDoubly_create_node(void * data);

/// @brief Aggiunge un nuovo nodo in fondo alla lista
/// @param head Puntatore al puntatore alla lista (anche vuota)
/// @param data dato del nodo
/// @return L'indirizzo del nodo allocato altrimenti NULL
DNode *listDoubly_append(DNode ** head, void *data);

/// @brief Aggiunge un nuovo nodo in testa alla lista
/// @param head Puntatore al puntatore alla lista (anche vuota)
/// @param data dato del nodo
/// @return L'indirizzo del nodo allocato altrimenti NULL
DNode *listDoubly_insertHead(DNode ** head, void * data);

/// @brief Estrae un nodo dalla testa della lista
/// @param head Puntatore al puntatore alla lista
/// @return Il puntatore al nodo appena estratto altrimenti NULL
DNode *listDoubly_extractHead(DNode ** head);

/// @brief Inserzione ordinata
/// @param head Puntatore al puntatore alla lista
/// @param data dato del Nodo
/// @param compare Funzione che determina l'ordinamento comparando i nodi
/// @return Il puntatore al nodo appena creato, altrimenti NULL
DNode * listDoubly_insert(DNode ** head, void *data, int (compare)(void *, void *));

/// @brief Conta il numero di nodi in una lista
/// @param head Puntatore alla testa della lista
/// @return il numero di nodi della lista
int listDoubly_count(DNode * head);

/// @brief Distrugge una lista
/// @param node Il puntatore alla testa della lista
/// @param release Funzione per deallocare il campo data
void listDoubly_destroy(DNode *node, void (*release)(void *));

/// @brief Stampa il contenuto della lista a video (DEBUG)
/// @param head Il puntatore alla testa della lista
/// @param print Funzione per stampare il contenuto del campo data
void listDoubly_print(DNode *head, void (*print)(void *));

/// @brief Funzione per stampare il campo data (stringhe)
/// @param data Puntatore data della lista
void listDoubly_print_string(void *data);

/// @brief Compara un nodo con un'altro
/// @param a Primo nodo da comparare
/// @param b Secondo nodo da comparare
/// @param compare Funzione che compara il contenuto dei due Nodi
/// @return minore di 0 se il a < b, 0 se a == b, maggiore di 0 se a a > b
int dNode_compare(DNode *a, DNode *b, int (compare) (void *, void *) );

/// @brief Aggiorna la posizione di un singolo elemento appena modificato (O(n))
/// @param elem Elemento della lista che è stato aggiornato
/// @param compare Funzione che compara il contenuto dei due Nodi
/// @return Ritorna true se una modifica è stata effettuata
bool listDoubly_sortElement(DNode * elem, int (compare) (void *, void *));

/// @brief Scambia la posizione di due nodi
/// @param a Primo nodo
/// @param b Secondo nodo
void dNode_exchange(DNode *a, DNode *b);

/********** DEBUG **********/

/// @brief Stampa il puntatore come fosse un intero (DEBUG)
/// @param data Puntatore contenente un intero
void listDoubly_print_intptr(void * data);

/// @brief Funzione comparazione, usa i puntatori come fossero interi (DEBUG)
/// @param a Primo valore 
/// @param b Secondo valore
/// @return <0 se a minore di b, 0 se a uguale a b, >0 se a maggiore di b
int intptr_compare(void *a, void *b);