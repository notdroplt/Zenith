#ifndef ZENITH_ELF_CREATOR
#define ZENITH_ELF_CREATOR 1

#if __has_include(<elf.h>)
#   include <elf.h>
#else 
#   error "ELF header file not found"
#endif 
#include <platform.h>
#include <stdint.h>

/**
 * @brief struct that holds an ELF file layout
 * 
 * Size 136 bytes;
 * 
 */
struct elf_layout_t {
    Elf64_Ehdr header; //!< ELF file header
    Elf64_Phdr program_headers[2]; //!< ELF program headers
    void ** section_contents; //!< file content
    void * section_strings; //!< section string array
    Elf64_Shdr * section_headers; //!< ELF section header
    uint64_t strings_size;
    uint16_t section_counter;
};

/**
 * @brief create the layout struct
 * 
 * @return struct elf_layout_t* malloc'd layout, or NULL on error
 */
CLink struct elf_layout_t * create_elf();

/**
 * @brief generate a section into the elf file
 * 
 * @param [in, out] layout the layout struct
 * @param [in] section_name section name to create
 * 
 * @return a section header 
 */
CLink Elf64_Shdr * generate_section(struct elf_layout_t * layout, char * section_name);


/**
 * @brief appends content to a section
 * 
 *
 * @param [in, out] layout the layout struct
 * @param [in] section_name section to append data
 * @param [in] data pointer to start of data to append
 * @param size data size
 *
 * @returns 0 on sucess
 * @returns 1 if section doesn't exist
 * @returns 2 if section couldn't be appended
 */
CLink int append_content_to_section(struct elf_layout_t * layout, char * section_name, void * data, uint64_t size);

/**
 * @brief writes header to file and free's the struct, so it is unusable after this call
 * 
 * @param [in] layout the layout struct
 * @param [in] filename filename to write to
 *
 * @return 0 on sucess
 * @returns 1 on failure
 */
CLink int write_to_file(struct elf_layout_t * layout, const char * filename);

#endif 