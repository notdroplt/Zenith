#pragma once
#ifndef ZENITH_TYPES_H
#define ZENITH_TYPES_H 1

#include "platform.h"
#include <stdint.h>
#include <stddef.h>
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
struct Array;

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
 * @brief creates an array with defined size
 * 
 * Complexity: depends on malloc
 *
 * @param size size to create the array (entries)
 * @return 
 */
struct Array* create_array(uint32_t size);

/**
 * @brief index an item in the array
 * 
 * Complexity: constant
 * 
 * @param [in] array array to be indexed
 * @param index index of the array
 * @return void* value at desired index
 */
const void * array_index(const struct Array* array, const uint64_t index);

/**
 * @brief sets a value at an index in a array
 * 
 * @param arr array to be used
 * @param index index to be set
 * @param value value to be set
 */
void array_set_index(struct Array * arr, const uint64_t index, void * value);

/**
 * @brief returns the size of an array
 * 
 * Complexity: Constant
 * 
 * @param arr array to check size
 * @return uint32_t size of array
 */
uint32_t array_size(const struct Array * arr);


/**
 * @brief copies like memcpy
 * 
 * Complexity: linear
 * 
 * @param arr array (destination)
 * @param ptr source pointer
 * @param size source pointer size
 * 
 * @returns 0 on success
 * @returns 1 on error 
 */
int array_copy_ptr(struct Array * arr, void ** ptr, uint64_t size);

/**
 * @brief deletes an object array
 *
 * @param [in] array array to be deleted 
 * @param [in] deleter deleter function
 */
void delete_array(struct Array* array, deleter_func deleter);

/**
 * @brief creates a list object
 * 
 * Complexy: depends on malloc
 * 
 */
struct List * create_list();

/**
 * @brief appends an item to a list
 * 
 * @param [in, out] list list to append item to
 * @param [in] item item to be appended
 * @returns 1 on error
 * @returns 0 on sucess
 */
int list_append(struct List * list, void * item);

/**
 * @brief transforms a linked list into an array
 * 
 * @param [in] list ist to be transformed
 * @return struct Array* returned array
 * 
 * @note the function does delete the list after finishing
 */
struct Array * list_to_array(struct List * list);

/**
 * @brief deletes a list
 * 
 * @param [in] list list to be deleted
 * @param [in] deleter function that destroys list objects, if it is NULL nothing happens
 */
void delete_list(struct List * list, deleter_func deleter);

/**
 * @brief pair struct used on hashmaps
 * 
 */
struct pair_t {
	void * first; //!< first value (key)
	void * second; //!< second value (value)
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
struct HashMap * create_map(uint64_t prealloc);

/**
 * @brief adds an integer key into the map
 * 
 * @param [in,out] map map to add key
 * @param key key to index
 * @param [in] value value to set
 * @returns -1 on fail, key value on success
 */
int map_addi_key(struct HashMap * map, uint64_t key, void * value);

/**
 * @brief adds a string key into the map
 * 
 * @param [in,out] map map to add key
 * @param [in] key key to index
 * @param [in] value value to set
 * @returns -1 on fail, key value on success
 */
int map_adds_key(struct HashMap * map, const char * key, void * value);

/**
 * @brief adds a string_t key into the map
 * 
 * @param [in,out] map map to add key
 * @param key key to index
 * @param [in] value to be added
 *  
 * @returns -1 on fail, key value on success
 */
int map_addss_key(struct HashMap * map, struct string_t key, void * value);

/**
 * @brief get a key:value from a map, using an integer
 * 
 * @param [in] map map to be keyed
 * @param key key to fetch
 * 
 * @returns pointer to pair, will never return a null pointer but
 * can return a reference to a 0:0 pair
*/
struct pair_t *map_getkey_i(const struct HashMap * map, const uint64_t key);

/**
 * @brief get a key:value from a map, using a string
 * 
 * @param [in] map map to be keyed
 * @param key key to fetch
 * 
 * @returns pointer to pair, will never return a null pointer but
 * can return a reference to a 0:0 pair
*/
struct pair_t *map_getkey_s(const struct HashMap * map, const char * key);

/**
 * @brief deletes map and its entries
 * 
 * @param [in] map map to be deleted
 * @param [in] deleter deleter function that is called on values
 */
void delete_map(struct HashMap * map, deleter_func deleter);
/*!< @} */ // end of group 
#endif
