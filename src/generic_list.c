#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "generic_list.h"


doubleLinkedList *list_create(dataCompareFunction data_cmp, typeAllocatorFunction type_alloc, typeDeallocatorFunction type_dealloc) {  
    doubleLinkedList *dllist = malloc(sizeof(doubleLinkedList));
    assert(dllist);
    dllist->head = NULL;
    dllist->tail = NULL;
    dllist->data_cmp = data_cmp;
    dllist->type_alloc = type_alloc;
    dllist->type_dealloc = type_dealloc;
    return dllist;
}

listNode *list_data_search(doubleLinkedList *dllist, const void *data) {
    assert(dllist);
    for (listNode *seeking = dllist->head; seeking != NULL; seeking = seeking->next) {
		if (dllist->data_cmp(data, seeking->data) == 0) {
			return seeking;
        }
    }
	return NULL;
}

uint8_t list_is_empty(doubleLinkedList *dllist) {
    assert(dllist);
    return (dllist->head == NULL);
}

listNode *list_last_node(doubleLinkedList  *dllist) {  
    assert(dllist);
    return dllist->tail;
}

void *list_get_item(doubleLinkedList *dllist, listNode *dllnode) {
    assert(dllist || dllnode);
    return dllnode->data;
}

listNode *list_last_insert(doubleLinkedList *dllist, void *data) {   
    assert(dllist);
    listNode *newNode = malloc(sizeof(listNode));
    assert(newNode);
    newNode->next = NULL;
    newNode->prev = NULL;
    newNode->data = dllist->type_alloc(data);

    if (!dllist->head) {
        dllist->head = newNode;
        dllist->tail = newNode;
    }
    else {
        dllist->tail->next = newNode;
        newNode->prev = dllist->tail;
        dllist->tail = newNode;
    }
    return newNode;
}

void list_iterate(doubleLinkedList *dllist, listVisitFunction visit) {   
    assert(dllist || visit);
	for (listNode *node = dllist->head; node != NULL; node = node->next)
		visit(dllist, node);
}

void list_delete_node(doubleLinkedList *dllist, listNode *del) { 
    assert(dllist);
    if (!dllist->head || !del) { return; }
    if (del == dllist->head) { dllist->head = dllist->head->next; }
    else if (del == dllist->tail) {
        dllist->tail = dllist->tail->prev;
        del->prev->next = NULL;
    }
    else if (del->next)
        del->prev->next = del->next;

    dllist->type_dealloc(del->data);
    free(del);
}

void list_destroy(doubleLinkedList *dllist) {  
    if(!dllist) { return; }
    listNode *current = dllist->head;
    while (current) {
        listNode *next = current->next;
        dllist->type_dealloc(current->data);
        free(current);
        current = next;
    }
    free(dllist);
}