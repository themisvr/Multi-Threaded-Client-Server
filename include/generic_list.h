#ifndef DOUBLE_LINKED_LIST_H
#define DOUBLE_LINKED_LIST_H

#include <stdint.h>
#include "alloc_funcs.h"


typedef int (*dataCompareFunction) (const void *ptr_a, const void *ptr_b);
typedef void *(*typeAllocatorFunction) (void *data);
typedef void (*typeDeallocatorFunction) (void *data);

typedef struct node {
    struct node *next;
    struct node *prev;
    void *data;
} listNode;

typedef struct list {
    struct node *head;
    struct node *tail;
    dataCompareFunction data_cmp;
    typeAllocatorFunction type_alloc;
    typeDeallocatorFunction type_dealloc;
} doubleLinkedList;

typedef void (*listVisitFunction) (doubleLinkedList *dllist, listNode *dllnode);


// creates an empty list
doubleLinkedList *list_create(dataCompareFunction data_cmp, typeAllocatorFunction type_alloc, 
								typeDeallocatorFunction type_dealloc);

// returns true if list is empty
uint8_t list_is_empty(doubleLinkedList *dllist);

// searching the given item and returns a pointer to its node
listNode *list_data_search(doubleLinkedList *dllist, const void *data);

// returns the last node of the list
listNode *list_last_node(doubleLinkedList *dllist);

// returns a pointer to the data
void *list_get_item(doubleLinkedList *dllist, listNode *dllnode);

// inserts an item at the end of the list with the given data
listNode *list_last_insert(doubleLinkedList *dllist, void *data);

// iterates the list, calling visit for each node
void list_iterate(doubleLinkedList *dllist, listVisitFunction visit);

// deletes an item from the list with the given data
void list_delete_node(doubleLinkedList *dllist, listNode *del);

// deallocates the whole memory, that has been allocated by the list
void list_destroy(doubleLinkedList *dllist);

#endif //DOUBLE_LINKED_LIST_H