#pragma once

#include <stdint.h>
#include <stddef.h>

#ifndef ARENA_DEFAULT_ALLOC_SIZE
#define ARENA_DEFAULT_ALLOC_SIZE 256
#endif

/**
 * @brief a pointer that might exist
 */
struct optional_pointer {
	void * pointer;
	int exists;
};

/**
 * @brief arena allocator struct
 *
 */
struct arena_allocator {
    void * base; /*!< base arena pointer */
    size_t size; /*!< how much the arena is holding */
    size_t capacity; /*!< how much the arena can hold */
};

enum arena_flags {
	A_ZMEM = 1 << 0, /*!< clear all memory allocated, default is to return as-is */
	A_SOFT = 1 << 1, /*!< do NOT abort whem arena runs out */
};

/**
 * @brief Create an arena allocator
 *
 * @return struct arena_allocator* allocated arena
 */
struct arena_allocator * create_arena(size_t size);

#define new_default_arena create_arena(0)

/**
 * @brief allocate inside an arena
 *
 * @param[in] arena arena to allocate into
 * @param size size of pointer to allocate
 * @param aligmnent aligment of data to point to
 * @param count amount of elements to allocate
 * @param flags flags used by the arena
 * @return arena_pointer arena inside pointer
 */
__attribute__((malloc, alloc_size(2, 4), alloc_align(3), nonnull(1)))
void * arena_alloc(struct arena_allocator * arena, size_t size, ptrdiff_t aligmnent, size_t count, enum arena_flags flags);

#define new(...) newx(__VA_ARGS__,new4,new3,new2)(__VA_ARGS__)
#define newx(a, b, c, d, e, ...) e
#define new2(a, t)          (t *)arena_alloc(a, sizeof(t), alignof(t), 1, 0)
#define new3(a, t, n)       (t *)arena_alloc(a, sizeof(t), alignof(t), n, 0)
#define new4(a, t, n, f)    (t *)arena_alloc(a, sizeof(t), alignof(t), n, f)

#define clear_arena(arena) \
	do                     \
		(arena)->size = 0; \
	while (0)

/**
 * @brief destroy an arena allocator
 *
 * @param arena arena to destroy
 */
void destroy_arena(struct arena_allocator *arena);
