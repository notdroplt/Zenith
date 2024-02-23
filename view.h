#pragma once

#include <stdint.h>
#include <string.h>

#if defined __has_builtin
#if __has_builtin(__builtin_constant_p)
#endif
#endif

#define view_name(type) type##_view
#define view_sig(type) struct view_name(type)

#define template_view(type) \
    view_sig(type)          \
    {                       \
        type *ptr;          \
        size_t size;        \
    }

#define view_size(type) sizeof(view_sig(type))

typedef template_view(void) default_view;

#define view_struct(array, s) \
    {                         \
        .ptr = array,         \
        .size = s,            \
    }

#define view_comp_struct(array)                       \
    {                                                 \
        .ptr = array,                                 \
        .size = (sizeof(array) / sizeof((array)[0])) \
    }

#define view_from_string_lit(string)						\
	{														\
		.ptr = string,										\
		.size = sizeof(string)/sizeof(char)-sizeof(char),	\
	}

#define create_view(type, array, size) \
    ((view_sig(type))view_struct(array, size))

#define null_view(type) \
	((view_sig(type)){  \
		.ptr = NULL,    \
		.size = 0       \
	})

#define view_compare(left, comp, right) \
	((left

#define generate_subview(type, view, start, vsize)    \
    ((view_sig(type)){                               \
        .ptr = (view)->ptr + (start * sizeof(type)), \
        .size = vsize,                                \
    })

#define view_pointer(type, view, index) ((view)->ptr + (index) * sizeof(type))

#define view_index(type, view, index) (*view_pointer(type, view, index))

#define view_walk(type, view)        \
    do                               \
    {                                \
        (view)->ptr += sizeof(type); \
        (view)->size--;              \
    } while (0)

#define view_run(type, view, amount)			\
	do											\
	{											\
		(view)->ptr += sizeof(type) * (amount); \
		(view)->size -= amount;					\
	} while (0)									\


#define view_start(view) (*((view)->ptr))

#define subview_inits_view(type, view, subview) (memcmp((view)->ptr, (subview)->ptr, (subview)->size * sizeof(type)) == 0)

#define view_copy(src, dest) \
	do { \
		(dest)->ptr = (src)->ptr; \
		(dest)->size = (src)->size; \
	} while (0)


