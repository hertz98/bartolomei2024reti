#pragma once
#include <stdbool.h>

typedef struct Node {
    void *data;
    struct Node *next;
} Node;

Node * list_create_node(void *);
Node * list_append(Node **, void *);
Node * list_insertHead(Node **, void *);
Node * list_extractHead(Node **);
int list_count(Node *);

void list_destroy(Node *node, void (*release)(void *));

void list_print(Node *head, void (*print)(void *));
void list_print_string(void *);