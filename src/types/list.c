#include <stdlib.h>
#include "types.h"

struct ListNode {
    struct ListNode * next;
    void * value;
};

struct List
{
    uint32_t size;
    struct ListNode * head;
    struct ListNode * tail;
};

struct List * create_list(void) {
    struct List * list = malloc(sizeof(struct List));
    if (!list) return NULL;

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;

}

int list_append(struct List * list, void *item) {    
    struct ListNode *new_node = malloc(sizeof(struct ListNode));
    if(!new_node) return 0;

    new_node->next = NULL;
    new_node->value = item;

    if (list->tail)
        list->tail->next = new_node;

    list->tail = new_node;

    if (!list->head) 
        list->head = list->tail;
    
    ++list->size;    
    return 1;
}

struct Array* list_to_array(struct List * list) {
    struct ListNode * node = list->head;
    struct Array * arr = create_array(list->size);
    if (arr == NULL) return NULL;

    
    for (uint32_t index = 0; node; ++index, node = node->next)
        array_set_index(arr, index, node->value);

    // don't delete list items, just the list and the indexes
    delete_list(list, NULL);
    
    return arr;
}

uint32_t list_size(const struct List * list) {
    return list->size;
}

void * list_get_tail_value(const struct List * list) {
    return &(list->tail->value);
}

void delete_list(struct List * list, deleter_func deleter) {
    struct ListNode * node, *tmp;
    if (!list->head) {
        free(list);
        return;
    }
    for (node = list->head; node;) {
        tmp = node;
        node = node->next;
        if (deleter)
            deleter(tmp->value);
        free(tmp);
    }
    free(list);
}


