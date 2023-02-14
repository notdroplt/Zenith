#include <stdlib.h>
#include <types.h>

struct ListNode {
    struct ListNode * next;
    const void * value;
};

struct List
{
    uint32_t size;
    struct ListNode * head;
};

struct List * create_list() {
    struct List * list = malloc(sizeof(struct List));
    if (!list) return NULL;

    list->head = NULL;
    list->size = 0;
    return list;

}

int append(struct List * list, const void *item) {
    
    ++list->size;
    if (list->head == NULL) {
        list->head = malloc(sizeof(struct ListNode));
        if (!list->head) return 1;
        list->head->next = NULL;
        list->head->value = item;
        return 0;
    }

    struct ListNode *node = list->head;
    for(; node->next; node = node->next);
    
    node->next = malloc(sizeof(struct ListNode));
    if(!node->next) return 1;
    node->next->next = NULL;
    node->next->value = item;

    return 0;
}

struct Array* to_array(const struct List * list) {
    struct Array * arr = create_array(list->size);
    if (arr == NULL) return NULL;

    struct ListNode * node = list->head;
    uint32_t index = 0;
    
    while (node)
    {
        array_set_index(arr, index, node->value);
        node = node->next;
        ++index;
    }
    
    return arr;
}