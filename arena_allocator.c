#include "arena_allocator.h"
#include <stdlib.h>
#include <string.h>

struct arena_allocator * create_arena(size_t default_size){
    struct arena_allocator * arena = malloc(sizeof(struct arena_allocator));
    if (!arena) return NULL;
    arena->base = malloc(default_size ? default_size : ARENA_DEFAULT_ALLOC_SIZE);
    if (!arena->base) {
        free(arena);
        return NULL;
    }

    arena->size = 0;
    arena->capacity = default_size ? default_size : ARENA_DEFAULT_ALLOC_SIZE;
    return arena;
}

void* arena_alloc(struct arena_allocator * arena, size_t size, ptrdiff_t alignment, size_t count, enum arena_flags flags) {
    ptrdiff_t padding = -(uintptr_t)arena->base & (alignment - 1);
	if (arena->size + (size * count + padding) > arena->capacity) {
        return NULL;
	}

    void* ptr = (void*)(arena->size + padding);

	if (flags & A_ZMEM)
		memset(ptr, 0, size * count);

    arena->size += size;
    return ptr;
}

void destroy_arena(struct arena_allocator * arena) {
    free(arena->base);
    free(arena);
}

