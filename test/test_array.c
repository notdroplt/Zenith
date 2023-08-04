#include <stdio.h>
#include <stdlib.h>
#include <types/types.h>

int main(void) {
    uint16_t size = rand();
    struct array_t * arr = create_array(size);
    if (array_size(arr) != size) {
        (void)fprintf(stderr, "could not allocate an array of defined size\n");
        return 1;
    }

    for (uint32_t i = 0; i < size; ++i) {
        void * value = (void *)(uint64_t)rand();
        array_set_index(arr, i, value);
        if (array_index(arr, i) != value) {
            (void)fprintf(stderr, "array failed to index\n");
            return 1;
        }
    }

    delete_array(arr, NULL);

    return 0;
}
