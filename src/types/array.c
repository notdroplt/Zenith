#include <stdlib.h>
#include <types.h>
#include <string.h>

struct Array {
    const void ** ptr;
    uint32_t size;
};

struct Array* create_array(uint32_t size) {
    struct Array* arr = malloc(size);
    if (!arr) return NULL;

    arr->ptr = malloc(size * sizeof(void *));
    arr->size = size;
    return arr;
}

const void * array_index(struct Array * arr, uint64_t index){
    return arr->ptr[index];
}

uint32_t array_size (struct Array * arr) {
    return arr->size;
}

void array_set_index(struct Array * arr, uint64_t index, const void * value) {
    arr->ptr[index] = value;
}

int array_copy_ptr(struct Array * arr, void ** ptr, uint64_t size ){
    if (size != arr->size) return 1;
    memcpy(arr->ptr, ptr, size * sizeof(void *));
    return 0;
}


void delete_array(struct Array * arr) {
    free(arr->ptr);
    free(arr);
}

