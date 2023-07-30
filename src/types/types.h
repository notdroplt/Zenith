#pragma once
#ifndef ZENITH_TYPES_H
#define ZENITH_TYPES_H 1

#include <string.h>
#include <platform.h>
/* default: SipHash-2-4 */
#ifndef cROUNDS
#define cROUNDS 2
#endif
#ifndef dROUNDS
#define dROUNDS 4
#endif
/**
 * @ingroup col_types Collection types
 * @brief defines a group of collections that are used on some of the code
 * 
 * @warning be aware that when the collections are deleted, it doesnt delete
 * any kind of members unless a deleter is passed
 * 
 * @{
 */

/**
 * @brief struct for an array type
 * 
 * in this case, the array elements are sequential
 */
struct array_t
{
    void **ptr;
    uint32_t size;
    uint32_t type;
} __attribute__((aligned(16)));
/**
 * @brief struct for a list type
 * 
 * in this case, the list is a doubly linked list
 */
struct List;

/**
 * @brief callback for deleter functions
 * 
 */
typedef void (*deleter_func)(void *);

/**
 * @brief a function that compares two objects
 *
 * @param v1 first value
 * @param v2 second value
 * 
 * @returns 0 when both values are equal
 * @returns non-0 when they're not
 * 
*/
typedef int (*comparer_func)(const void *v1, const void *v2);

/**
 * @brief creates an array with defined size
 * 
 * Complexity: depends on malloc
 *
 * @param size size to create the array (entries)
 * @return 
 */
struct array_t *create_array(uint32_t size);

/**
 * @brief index an item in the array
 * 
 * Complexity: constant
 * 
 * @param [in] array array to be indexed
 * @param index index of the array
 * @return void* value at desired index
 */
const void *array_index(const struct array_t *array, const uint64_t index);

/**
 * @brief sets a value at an index in a array
 * 
 * @param arr array to be used
 * @param index index to be set
 * @param value value to be set
 */
void array_set_index(struct array_t *arr, const uint64_t index, void *value);

/**
 * @brief returns the size of an array
 * 
 * Complexity: Constant
 * 
 * @param arr array to check size
 * @return uint32_t size of array
 */
uint32_t array_size(const struct array_t *arr);

/**
 * @brief finds an item in the array
 * 
 * Complexity: Linear (not accounting time for function to be ran)
 * 
 * @param [in] arr array that will be searched
 * @param val value to be compared 
 * @param [in] comp a comparision function that returns 0 when both values are equal and non-zero when they are different
 * 
 * @returns -1 if not found
 * @returns index if found
*/
int32_t array_find(const struct array_t *arr, const void *cmp, comparer_func comp);

/**
 * @brief copies like memcpy
 * 
 * Complexity: Linear
 * 
 * @param arr array (destination)
 * @param ptr source pointer
 * @param size source pointer size
 * 
 * @returns 0 on success
 * @returns 1 on error 
 */
int array_copy_ptr(struct array_t *arr, void **ptr, uint64_t size);

/**
 * @brief returns ther raw pointer to the array's start
 * 
 * Complexity: Constant
 *
 * @param arr array pointer
 * @return void* raw pointer
 */
void *array_get_ptr(const struct array_t *arr);

/**
 * @brief compares two arrays
 * 
 * @param left left array to compare
 * @param right right array to compare
 * @param comp comparator function
 * @return true when same
 * @return false when different 
 */
bool array_compare(const struct array_t *left, const struct array_t *right, comparer_func comp);

/**
 * @brief deletes an object array
 *
 * @param [in] array array to be deleted 
 * @param [in] deleter deleter function
 */
void delete_array(struct array_t *array, deleter_func deleter);

/**
 * @brief creates a list object
 * 
 * Complexy: depends on malloc
 * 
 */
struct List *create_list(void);

/**
 * @brief appends an item to a list
 * 
 * @param [in, out] list list to append item to
 * @param [in] item item to be appended
 * @returns 0 on error
 * @returns 1 on success
 */
int list_append(struct List *list, void *item);

/**
 * @brief transforms a linked list into an array
 * 
 * @param [in] list ist to be transformed
 * @return struct array_t* returned array
 * 
 * @note the function does delete the list after finishing
 */
struct array_t *list_to_array(struct List *list);

/**
 * @brief returns array size
 * 
 * @param [in] list current list
 * @return uint32_t list's size
 */
uint32_t list_size(const struct List *list);

/**
 * @brief get last value pointer
 * 
 * @param list 
 * @return void* value at last point on the list
 */
void *list_get_tail_value(const struct List *list);

/**
 * @brief deletes a list
 * 
 * @param [in] list list to be deleted
 * @param [in] deleter function that destroys list objects, if it is NULL nothing happens
 */
void delete_list(struct List *list, deleter_func deleter);

/**
 * @brief pair struct used on hashmaps
 * 
 */
struct pair_t {
    void *first;  /*!< first value (key) */
    void *second; /*!< second value (value) */
	uint64_t type; /*!< type for value key */
};

/**
 * @brief hash map foward definition
 * 
 */
struct HashMap;

/**
 * @brief hashes a string into a uint64_t
 * 
 * @param in string to be hashed
 * @param inlen size of string to be hashed
 * @param k key to be hashed
 * @return hashed value for string
 */
uint64_t siphash(const void *in, const size_t inlen, const void *k);

/**
 * @brief create a hashmap with a table size
 * 
 * @param prealloc amount of entries to allocate (if 0 defaults to 32)
 * @return struct HashMap* generated map
 */
struct HashMap *create_map(uint64_t prealloc);

/**
 * @brief adds an integer key, with a "typed" value, into the map
 * 
 * @param [in,out] map map to add key
 * @param key key to index
 * @param type value type
 * @param [in] value value to set
 * @returns -1 on fail, key value on success
 */
int map_addit_key(struct HashMap *map, uint64_t key, uint64_t type, void *value);

/**
 * @brief adds a string key, with a "typed" value, into the map
 * 
 * @param [in,out] map map to add key
 * @param [in] key key to index
 * @param type value type
 * @param [in] value value to set
 * @returns -1 on fail, key value on success
 */
int map_addst_key(struct HashMap *map, const char *key, uint64_t type, void *value);

/**
 * @brief adds a string_t key into the map
 * 
 * @param [in,out] map map to add key
 * @param key key to index
 * @param [in] value to be added
 *  
 * @returns -1 on fail, key value on success
 */
int map_addsst_key(struct HashMap *map, struct string_t key, uint64_t type, void *value);

/**
 * @brief adds an integer key into the map
 * 
 * @param [in,out] map map to add key
 * @param key key to index
 * @param [in] value value to set
 * @returns -1 on fail, key value on success
 */
#define map_addi_key(map, key, value) map_addit_key(map, key, 0, value)

/**
 * @brief adds a string key into the map
 * 
 * @param [in,out] map map to add key
 * @param [in] key key to index
 * @param [in] value value to set
 * @returns -1 on fail, key value on success
 */
#define map_adds_key(map, key, value) map_addst_key(map, key, 0, value)

/**
 * @brief adds a string_t key into the map
 * 
 * @param [in,out] map map to add key
 * @param key key to index
 * @param [in] value to be added
 *  
 * @returns -1 on fail, key value on success
 */
#define map_addss_key(map, key, value) map_addsst_key(map, key, 0, value)

/**
 * @brief get a key:value from a map, using an integer
 * 
 * @param [in] map map to be fetch
 * @param key key to fetch
 * 
 * @returns pointer to pair, will never return a null pointer but
 * can return a reference to a 0:0 pair
*/
struct pair_t *map_getkey_i(const struct HashMap *map, const uint64_t key);

/**
 * @brief get a key:value from a map, using a string
 * 
 * @param [in] map map to be fetch
 * @param key key to fetch
 * 
 * @returns pointer to pair, will never return a null pointer but
 * can return a reference to a 0:0 pair
*/
struct pair_t *map_getkey_s(const struct HashMap *map, const char *key);

/**
 * @brief get a key:value from a map
*/
struct pair_t *map_getkey_ss(const struct HashMap *map, const struct string_t key);

/**
 * @brief iterates over map keys
 * 
 * @param map map to iterate over
 * @param value value to compare
 * @param function compare function
 */
void map_iterate(struct HashMap *map, void *value, comparer_func function);

/**
 * @brief deletes map and its entries
 * 
 * @param [in] map map to be deleted
 * @param [in] deleter deleter function that is called on values
 */
void delete_map(struct HashMap *map, deleter_func deleter);

struct TypedHashMap;

/*!< @} */ /* end of group */
#endif
