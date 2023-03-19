#include <types.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    uint16_t size = rand();
    struct Array * arr = create_array(size);
    if (array_size(arr) != size) {
        fprintf(stderr, "could not allocate an array of defined size\n");
        return 1;
    }

    for (uint32_t i = 0; i < size; ++i) {
        void * value = (void *)(uint64_t)rand();
        array_set_index(arr, i, value);
        if (array_index(arr, i) != value) {
            fprintf(stderr, "array failed to index\n");
            return 1;
        }
    }

    delete_array(arr, NULL);

    return 0;
}
