#pragma once
#ifndef ZENITH_ELF_CREATOR
#define ZENITH_ELF_CREATOR 1

#include "platform.h"
#include "types.h"

/**
 * @brief symbol counting
 *
 */
typedef char *zst_name_t;      /*!< name string */
typedef uint32_t zst_count_t;  /*!< counter type*/
typedef uint64_t zst_offset_t; /*!< offset */
typedef uint64_t zst_size_t;   /*!< size */
typedef uint64_t zst_hash_t;   /*!< name hash */

/**
 * @brief flags for diferent symbols
 *
 */
enum zst_section_flags
{
    sec_read = 1,    /*!< section can be read to */
    sec_write = 2,   /*!< section can be written to */
    sec_execute = 4, /*!< section can be executed */
    sec_zero = 8,    /*!< section needs zero'd memory */
    sec_copy = 16,   /*!< section needs to be copied to memory */
    sec_noop = 32    /*!< section isn't loaded */
};

/**
 * @brief reference an undefined item
 * @note `section_off` points to the first byte where it is referenced
 */
struct reference_t
{
    zst_name_t name;          /*!< reference name */
    zst_offset_t section_off; /*!< offset of reference on symbol */
};

/**
 * @brief
 */
struct zst_symbol_t
{
    zst_name_t name;             /*!< symbol name */
    zst_offset_t start_addr;     /*!< symbol start address */
    zst_size_t size;             /*!< symbol start address */
    zst_offset_t references;     /*!< pointer to references */
    zst_offset_t content;        /*!< pointer to content */
    zst_count_t reference_count; /*!< reference count */
};

struct zst_section_t
{
    zst_name_t name;                  /*!< section name */
    enum zst_section_flags flags;     /*!< section flags */
    zst_size_t file_size;             /*!< size on file */
    zst_size_t physical_size;         /*!< size on memory */
    zst_offset_t file_off;            /*!< offset on file */
    zst_offset_t physical_off;        /*!< offset on memory */
    zst_count_t symbols;              /*!< count of symbols defined */
    zst_count_t undefined_references; /*!< count of undefined references */
};

enum target_architecture
{
    zst_target_invalid,
    zst_target_zenithvm
};

/**
 * @brief defines expectations to the target
 *
 */
enum target_expects
{
    zst_expects_nothing = 0,              /*!< there are no expectations to target */
    zst_expects_clock_speed = 1,          /*!< there is a set clock speed (in Hz, defined later)*/
    zst_expects_ram_size = 2,             /*!< there is a defined ram size (in bytes, defined later) */
    zst_expects_rom_size = 4,             /*!< there is a defined rom size (in bytes, defined later) */
    zst_expects_freestanding = 8,         /*!< there is no host enviroment on target */
    zst_expects_big_endian = 16,          /*!< target uses big endian */
    zst_expects_stack_depth = 32,         /*!< there is a set stack depth to be used (counts, defined later)*/
    zst_expects_vector_instructions = 64, /*!< target *should* support vector instructions */
    zst_expects_multithreading = 128      /*!< target *should* be able to run in more than one thread */
};

struct zst_header_t
{
    uint32_t magic; /*!< file magic*/
    struct
    {
        uint8_t major; /*!< major file version */
        uint8_t minor; /*!< minor file version */
    } version; /*!< current file version (does not refer to the compiler version) */

    enum target_expects expectations; /*!< bitfield with compiler expectations */

    struct
    {
        uint64_t clock_speed; /*!< expected clock speed (in Hz) */
        uint64_t ram_size; /*!< expected ram size (in bytes) */
        uint64_t rom_size; /*!< expected rom size (in bytes) */
        uint64_t stack_depth; /*!< stack depth value (counted) */
    } expect; /*!< value of the compiler expectations */

    zst_offset_t entry_point; /*!< entry point (on physical memory)*/
    zst_count_t entries; /*!< count of entries on the file*/
};

#define ZSF_HEADER_MAG 0x0046485A

/* THESE MACROS DON'T REFER TO THE SAME VERSION AS THE COMPILER */
#define ZSF_HEADER_VER_MAJ 1
#define ZSF_HEADER_VER_MIN 0

struct zst_layout_t;

struct zst_layout_t *create_layout(uint8_t prealloc);

struct zst_section_t *create_section(struct zst_layout_t *layout, char *section_name);

struct zst_symbol_t *create_symbol_on_section(struct zst_layout_t *layout, char *name, struct Array *data, struct Array *references, char * section_name);

/**
 * @brief transforms a layout into a written array and deletes the given layout,
 *
 * the function
*/
struct Array * zsf_link_and_relocate(struct zst_layout_t * layout);

/**
 * @brief writes raw data into  .ihex file
 *
 * @param [in] data pointer pointing to data array
 * @param data_size size of data elements
 * @param [in] filename output file name
 * @return int error code
 */
int ihex_create_file(void *data, uint64_t data_size, const char *filename);

#endif
