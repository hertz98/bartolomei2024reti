#include <stdio.h>
#include <stdlib.h>
#include "list.h"

Node * create_node(void *data)
{
    Node *tmp = (Node *)malloc(sizeof(Node));

    if (!tmp)
    {
        perror("Failed to allocate memory");
        return NULL;
    }

    tmp->data = data;
    tmp->next = NULL;
    return tmp;
}

Node * append(Node ** head, void *data)
{
    Node *tmp = create_node(data);
    if (!tmp)
        return NULL;

    if (!(*head)) // Caso lista vuota
        *head = tmp;
    else
    {
        Node *current = *head;
        while (current->next)
            current = current->next;
        current->next = tmp;
    }

    return tmp;
}

void print_list(Node *head, void (*print)(void *))
{
    for(Node *tmp = head; tmp; tmp = tmp->next)
        print(tmp->data);
}

void destroy(Node *node, void (*release)(void *))
{
    if (!node)
        return;

    destroy(node->next, release);

    if (release)
        release(node->data);
    free(node);
}
