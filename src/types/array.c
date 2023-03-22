#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"

struct Array {
    uint32_t size;
    void ** ptr;
};

struct Array* create_array(uint32_t size) {
    struct Array* arr = malloc(sizeof(struct Array));
    if (!arr) return NULL;

    arr->ptr = malloc(size * sizeof(void *));
    arr->size = size;
    return arr;
}

const void * array_index(const struct Array * arr, const uint64_t index){
    return arr->ptr[index];
}

uint32_t array_size (const struct Array * arr) {
    return arr->size;
}

void array_set_index(struct Array * arr, const uint64_t index, void * value) {
    arr->ptr[index] = value;
}

int array_copy_ptr(struct Array * arr, void ** ptr, uint64_t size ){
    if (size != arr->size) return 1;
    memcpy(arr->ptr, ptr, size * sizeof(void *));
    return 0;
}

int32_t array_find(const struct Array * arr, const void * cmp, comparer_func comp) {
    for (size_t i = 0; i < arr->size; ++i)
        if (!comp(arr->ptr[i], cmp)) return i;
    return -1;    
    
}

bool array_compare(const struct Array * left, const struct Array * right, comparer_func comp) {
    if (left->size != right->size) return false;
    if (left->ptr == right->ptr) return true;

    for (register size_t i = 0; i < left->size; ++i) {
        if (!comp(left->ptr[i], right->ptr[i])) return false;
    }

    return true;
}

void delete_array(struct Array * arr, deleter_func deleter) {
    if (!deleter) {
        goto delete_end;
    }
    
    for (size_t i = 0; i < arr->size; ++i)
        if (arr->ptr[i])
            deleter(arr->ptr[i]);

delete_end:
    free(arr->ptr);
    free(arr);
}

void * array_get_ptr(const struct Array * arr) {
    return arr->ptr;
}
