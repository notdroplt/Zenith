#include <types.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    uint64_t size = rand() % 256;
    struct Array * arr = create_array(size);
    if (array_size(arr) != size) {
        fprintf(stderr, "could not allocate an array of defined size\n");
        return 1;
    }

    uint64_t index = rand() % size;
    uint64_t value = rand();
    array_set_index(arr, index, (void*)value);

    if (array_index(arr, index) != (void*)value) {
        fprintf(stderr, "array failed to index\n");
        return 1;
    }

    delete_array(arr);

    return 0;
}