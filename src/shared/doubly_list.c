#include <stdio.h>
#include <stdlib.h>
#include "doubly_list.h"

DNode * listDoubly_create_node(void *data)
{
    DNode *tmp = (DNode *)malloc(sizeof(DNode));

    if (!tmp)
    {
        perror("Failed to allocate memory");
        return NULL;
    }

    tmp->data = data;
    tmp->next = NULL;
    tmp->prev = NULL;
    return tmp;
}

DNode * listDoubly_append(DNode ** head, void *data)
{
    DNode *tmp = listDoubly_create_node(data);
    if (!tmp)
        return NULL;

    if (!(*head)) // Caso lista vuota
        *head = tmp;
    else
    {
        DNode *current = *head;
        while (current->next)
            current = current->next;
        tmp->prev = current;
        current->next = tmp;
    }

    return tmp;
}

DNode *listDoubly_insertHead(DNode ** head, void * data)
{
    DNode *tmp = listDoubly_create_node(data);
    if (!tmp)
        return NULL;

    DNode *next = *head;
    *head = tmp;
    (*head)->next = next;
    (*head)->next->prev = tmp;

    return tmp;
}

DNode *listDoubly_extractHead(DNode ** head)
{
    if (!*head)
        return NULL;

    DNode *tmp = *head;
    *head = tmp;
    *head = (*head)->next;
    (*head)->prev = NULL;

    return tmp;
}

DNode *listDoubly_insert(DNode **head, void *data, int(compare)(void *, void *))
{
    DNode *tmp = listDoubly_create_node(data);
    if (!tmp)
        return NULL;
    
    if (!(*head) || dNode_compare(*head, tmp, compare) >= 0 )
    {
        tmp->next = *head;
        if (*head)
             (*head)->prev = tmp;   
        *head = tmp;
    }
    else
    {
        DNode * current = *head;
        while (current->next && dNode_compare(tmp, current->next, compare) >= 0)
            current = current->next;
        
        tmp->prev = current;
        tmp->next = current->next;
        if (current->next)
            current->next->prev = tmp;
        current->next = tmp;
    }
    
    return tmp;
}

int inline listDoubly_count(DNode *head)
{
    int n = 0;
    for (DNode * tmp = head; tmp; tmp = tmp->next)
        n++;
    return n;
}

void listDoubly_print_string(void *data) {
    printf("%s", (char *) data);
}

void listDoubly_print_intptr(void *data)
{
    printf("%d", (int) (__intptr_t) data);
}

int dNode_compare(DNode *a, DNode *b, int(compare)(void *, void *))
{
    if (!a && !b)
        return 0;
    else if (!a)
        return -1;
    else if (!b)
        return 1;
    else
        return compare(a->data, b->data);
}

bool listDoubly_sortElement(DNode *elem, int(compare)(void *, void *))
{
    bool moved = false; // Dando per scontato una lista ordianta si sposta in un solo verso
    if (elem->next)
    {
        for (DNode *current = elem, *next = current->next; 
                next && dNode_compare(current, next, compare) > 0;
                current = current->next, next = next->next)
        {
            dNode_exchange(current, next);
            moved = true;
        }
    }
    if (!moved && elem->prev)
    {
        for (DNode *current = elem, *prev = current->prev; 
            prev && dNode_compare(current, prev, compare) < 0;
            current = current->prev, prev = prev->prev)
        {
            dNode_exchange(current, prev);
            moved = true;
        }
    }
    return moved;
}

// Mantengo la posizione ma scambio il campo data
// semplice, ma necessita cambiare i riferimenti
void dNode_exchange(DNode *a, DNode *b)
{
    void * tmp = a->data;
    a->data = b->data;
    b->data = tmp;
}

int intptr_compare(void *a_ptr, void *b_ptr)
{
    __intptr_t a = (__intptr_t) a_ptr;
    __intptr_t b = (__intptr_t) b_ptr;

    if (a < b)
        return -1;
    else if (a == b)
        return 0;
    else
        return 1;
}

void listDoubly_print(DNode *head, void (*print)(void *))
{
    for(DNode *tmp = head; tmp; tmp = tmp->next)
    {
        print(tmp->data);
        printf(" -> ");
    }
    printf("NULL\n");
}

void listDoubly_destroy(DNode *node, void (*release)(void *))
{
    if (!node)
        return;

    listDoubly_destroy(node->next, release);

    if (release)
        release(node->data);
    free(node);
}
