#include <stdbool.h>

typedef struct Node {
    void *data;
    struct Node *next;
} Node;

Node * create_node(void *);
Node * append(Node **, void *);
void print_list(Node *head, void (*print)(void *));
void destroy(Node *node, void (*release)(void *));