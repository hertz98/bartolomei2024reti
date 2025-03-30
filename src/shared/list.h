#pragma once

#ifndef LIST_HEADER
#define LIST_HEADER

#include <stdbool.h>

/********** LISTA GENERICA **********/

typedef struct Node {
    void *data; // Puntatore a qualsiasi tipo
    struct Node *next;
} Node;

/// @brief Alloca un nuovo nodo
/// @param data puntatore al dato che si vuole allocare 
/// @return Il nodo appena allocato, altrimenti NULL
Node *list_create_node(void * data);

/// @brief Aggiunge un nuovo nodo in fondo alla lista
/// @param head Puntatore al puntatore alla lista (anche vuota)
/// @param data dato del nodo
/// @return L'indirizzo del nodo allocato altrimenti NULL
Node *list_append(Node ** head, void *data);

/// @brief Aggiunge un nuovo nodo in testa alla lista
/// @param head Puntatore al puntatore alla lista (anche vuota)
/// @param data dato del nodo
/// @return L'indirizzo del nodo allocato altrimenti NULL
Node *list_insertHead(Node ** head, void * data);

/// @brief Estrae un nodo dalla testa della lista
/// @param head Puntatore al puntatore alla lista
/// @return Il puntatore al nodo appena estratto altrimenti NULL
Node *list_extractHead(Node ** head);

/// @brief Conta il numero di nodi in una lista
/// @param head Puntatore alla testa della lista
/// @return il numero di nodi della lista
int list_count(Node * head);

/// @brief Distrugge una lista
/// @param node Il puntatore alla testa della lista
/// @param release Funzione per deallocare il campo data
void list_destroy(Node *node, void (*release)(void *));

/// @brief Distrugge una lista cominciando dal primo elemento (la differnza può stare nella release)
/// @param node Il puntatore alla testa della lista
/// @param release Funzione per deallocare il campo data
void list_destroyPreorder(Node *node, void (*release)(void *));

/// @brief Stampa il contenuto della lista a video (DEBUG)
/// @param head Il puntatore alla testa della lista
/// @param print Funzione per stampare il contenuto del campo data
void list_print(Node *head, void (*print)(void *));

/// @brief Funzione per stampare il campo data (stringhe)
/// @param data Puntatore data della lista
void list_print_string(void *data);

#endif