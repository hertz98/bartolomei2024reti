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

bool listDoubly_sortElement(DNode **head, DNode *elem, int(compare)(void *, void *))
{
    bool moved = false;
    while (elem->next && 
            dNode_compare(elem, elem->next, compare) > 0 && 
            listDoubly_DNode_moveFordward(head, NULL, elem))
        moved = true;

    if (!moved)
        while (elem->prev && 
            dNode_compare(elem, elem->prev, compare) < 0 && 
            listDoubly_DNode_moveBack(head, NULL, elem))
        moved = true;

    return moved;
}

// Mantengo la posizione ma scambio il campo data
// semplice, ma necessita cambiare i riferimenti
void listDoubly_DNode_swap(DNode ** head, DNode ** tail, DNode *a, DNode *b)
{
    if (!a || !b)
        return;
    
    if (a == b)
        return;
    
    if (b->next == a) // Caso nodi consecutivi (inverso)
        return listDoubly_DNode_swap(head, tail, b, a);

    // Puntatori esterni
    if (a->prev)
        a->prev->next = b;
    else if (head)
        *head = b;

    if (b->next)
        b->next->prev = a;
    else if (tail)
        *tail = a;

    if (b->prev && b->next != b)
        b->prev->next = a;
    else if (head)
        *head = a;

    // Puntatori successivi
    if (a->next && a->next != b)
        a->next->prev = b;
    else if (tail)
        *tail = b;  

    // Puntatori interni

    if (a->next == b) // Caso nodi consecutivi
    {
        void * tmp = a->prev;
        a->next = b->next;
        a->prev = b;
        b->next = a;
        b->prev = tmp;
    }
    else // Caso generico nodi non consecutivi
    {        
        DNode tmp = *a;
        a->next = b->next;
        a->prev = b->prev;
        b->next = tmp.next;
        b->prev = tmp.prev;
    }

}

void listDoubly_DNode_swapData(DNode *a, DNode *b)
{
    void *tmp = a->data;
    a->data = b->data;
    b->data = tmp;
}

bool listDoubly_DNode_moveFordward(DNode ** head, DNode ** tail, DNode * elem)
{
    if (!elem)
        return false;

    DNode * toSwap = elem->next;

    if (!toSwap)
        return false;
        
    if (elem->prev)
        elem->prev->next = toSwap;
    else if (head)
        *head = toSwap;

    if (toSwap->next)
        toSwap->next->prev = elem;
    else if (tail)
        *tail = elem;
    elem->next = toSwap->next;
    toSwap->next = elem;
    toSwap->prev = elem->prev;
    elem->prev = toSwap;
    
    return true;
}

bool listDoubly_DNode_moveBack(DNode ** head, DNode ** tail, DNode * elem)
{
    if (!elem)
        return false;

    DNode * toSwap = elem->prev;

    if (!toSwap)
        return false;

    if (elem->next)
        elem->next->prev = toSwap;
    else if (tail)
        *tail = elem;

    if (toSwap->prev)
        toSwap->prev->next = elem;
    else if (head)
        *head = elem;
    elem->prev = toSwap->prev;
    toSwap->prev = elem;
    toSwap->next = elem->next;
    elem->next = toSwap;

    return true;
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
