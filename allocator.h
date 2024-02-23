/**
 * @brief define a file that should be able to switch between different allocation methods
 *
 * the default is set to malloc/free
 */

#pragma once
#ifndef ZENITH_ALLOCATOR_H
#define ZENITH_ALLOCATOR_H 1

#ifndef ALLOCATOR_NEEDS_LOCAL_POOL
//! define if the current allocator needs a local pool (kinda like a lifetime)
#define ALLOCATOR_NEEDS_LOCAL_POOL 0
#endif

#ifndef ALLOC_FREE_POINTER
//! for allocators that support freeing allocated pointers, such as heaps or pools but not arenas
#define ALLOC_FREE_POINTER 1
#endif

#ifndef ALLOC_FORCE_ALIGNMENT
#define ALLOC_FORCE_ALIGNMENT 0
#endif

#ifndef ALLOC_DESTROY_POOL
//! allocators that need to be destroyed after use, most implementations that don't rely on globals will set to 1
#define ALLOC_DESTROY_POOL 0
#endif

#if ALLOCATOR_NEEDS_LOCAL_POOL
#define allocator_param(pool, ...) pool, __VA_ARGS__
#else
#define allocator_param(pool, ...) __VA_ARGS__
#endif
#if ALLOCATOR_NEEDS_LOCAL_POOL && \
!(defined(init_alloc) && defined(alloc) && defined(dealloc) && defined(destroy_alloc)) \
&& (defined(init_alloc) || defined(alloc) || defined(dealloc) || defined(destroy_alloc))
#	error "all 'init_alloc', 'alloc', 'free', and 'destroy_alloc' should be defined"
#endif

#ifndef pool_t
typedef void * void_ptr;
#define pool_t void_ptr
#endif


#ifndef init_alloc
//! @brief define a function to generate a pool for allocation
//! because malloc/free does not rely on
//! context defined pools, the default is only a cast to NULL
#define init_alloc(size, flags) (pool_t)(NULL)
#endif

#ifndef alloc
//! @brief define a function to allocate data
//! @param allocator pointer to allocate
//! @param size size in bytes of allocated variable
//! @param alignment alignment of data in bytes
//! @param count how many times to allocate that much data
//! @param flags flags to the allocator
//!
//! @returns pointer to allocated data
#define alloc(allocator, size, alignment, count, flags) malloc((size) * (count));
#endif

#ifndef dealloc
//! @brief define a function to deallocate a pointer
//! @param allocator allocator pool
//! @param pointer pointer to deallocate
//! @param flags flags
#define dealloc(allocator, pointer, flags)	\
	do {									\
		(void)allocator;					\
		(void)flags;						\
		free(pointer);						\
	} while (0)
#endif

#ifndef destroy_alloc
//! @brief function to destroy a pool
#define destroy_alloc(allocator, flags) 0
#endif


#endif
