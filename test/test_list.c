#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct ListNode {
    struct ListNode * next;
    const void * value;
};

struct List
{
    uint32_t size;
    struct ListNode * head;
    struct ListNode * tail;
};

int main(void) {
    srand(time(NULL));

    struct List * list = create_list();
    if (!list) {
        fprintf(stderr, "could not initiate list\n");
        return 1;
    }

    /* limit size a bit */
    uint16_t size = rand();

    uint64_t *array = malloc(sizeof(uint64_t) * size);
    if (!array) {
        delete_list(list, NULL);
        fprintf(stderr, "could not allocate %ld bytes for comparision array\n", size * sizeof(uint64_t));
        return 1;
    }

    for (register uint32_t i = 0; i < size; ++i) {
        array[i] = rand();
        if (list_append(list, (void *)array[i])) {
            fprintf(stderr, "error while appending value %lu at index #%u\n", array[i], i);
            delete_list(list, NULL);
            return 1;
        }
    }

    /* because we have the size, and the pointer to the list goes first, we can do this without problems */
    struct Array * result_array = list_to_array(list);

    for (register uint16_t i = 0; i < size; ++i) {
        register uint64_t value = (uint64_t)array_index(result_array, i);
        if (value != array[i]) {
            fprintf(stderr, "error on index #%u, values differ: expected [%lu] and got [%lu]\n", i, array[i], value);
            return 1;
        }
    }

    delete_array(result_array, NULL);
    free(array);
    return 0;
}
