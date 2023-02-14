#ifndef ZENITH_TYPES_H
#define ZENITH_TYPES_H 1

#include <stdint.h>

/**
 * @ingroup col_types Collection types
 * @brief defines a group of collections that are used on some of the code
 * 
 * @warning be aware that when the collections are deleted, it doesnt delete
 * any kind of members
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
const void * array_index(struct Array* array, uint64_t index);

/**
 * @brief sets a value at an index in a array
 * 
 * @param arr array to be used
 * @param index index to be set
 * @param value value to be set
 */
void array_set_index(struct Array * arr, uint64_t index, const void * value);

/**
 * @brief returns the size of an array
 * 
 * Complexity: Constant
 * 
 * @param arr array to check size
 * @return uint32_t size of array
 */
uint32_t array_size(struct Array * arr);


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
 */
void delete_array(struct Array* array);

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
int append(struct List * list, const void * item);

/**
 * @brief transforms a linked list into an array
 * 
 * @param [in] list ist to be transformed
 * @return struct Array* returned array
 * 
 * @note the function does not delete the list
 */
struct Array * to_array(const struct List * list);

/**
 * @brief deletes a list
 * 
 * @param [in] list list to be deleted
 */
void delete_list(struct List * list);

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
 *
 * @param string string to be hashed
 * @return hashed value for string
 */
uint64_t cstr_hash(const char * string);

/**
 * @brief create a hashmap with a table size
 * 
 * @param prealloc amount of entries to allocate (if 0 defaults to 32)
 * @return struct HashMap* generated map
 */
struct HashMap * create_map(uint64_t prealloc);


/*!< @} */ // end of group 
#endif
