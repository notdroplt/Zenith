
#include <stdlib.h>
#include <string.h>

#include "formats.h"
struct linked_symbol;
struct linked_section;

struct linked_symbol {
    struct zst_symbol_t symbol; 
    struct linked_section * parent;
    struct Array * content;
    struct Array * references;
};

struct linked_section {
    struct zst_section_t section;
    struct linked_symbol ** symbols;
};

struct zst_layout_t {
    struct zst_header_t header;
    struct HashMap * section_map;
    struct HashMap * symbol_map;
};

/* not everything here is implemented yet */

struct zst_layout_t * create_layout(uint8_t prealloc) {
    struct zst_layout_t * layout = malloc(sizeof(struct zst_layout_t));
    struct zst_header_t * header = NULL;;
    if (!layout) return NULL;

    layout->section_map = create_map(prealloc);
    if (!layout->section_map) {
        free(layout);
        return NULL;
    }

    layout->symbol_map = create_map(0);
    if (!layout->symbol_map) {
        delete_map(layout->section_map, NULL);
        free(layout);
        return NULL;
    }

    header = &layout->header;

    header->magic = ZSF_HEADER_MAG;
    header->version.major = ZSF_HEADER_VER_MAJ;
    header->version.minor = ZSF_HEADER_VER_MIN;
    header->expectations = zst_expects_nothing;
    header->entries = prealloc;


    return layout;
}

struct zst_section_t * create_section(struct zst_layout_t* layout, char * section_name) {
    struct zst_section_t * section = NULL;
    struct linked_section * lsect = malloc(sizeof(struct linked_section));
    if (!lsect) return NULL;
    section = &lsect->section;
    
    section->name = section_name;
    section->file_off = 0; 
    section->file_size = 0; 
    section->flags = 0;
    section->physical_off = 0;
    section->physical_size = 0;
    section->symbols = 0;
    section->undefined_references = 0;

    map_adds_key(layout->section_map, section_name, lsect);

    /* structs don't have padding on the first member, */
    /* so returning the section is valid*/
    return section;
}

struct zst_symbol_t * create_symbol_on_section(struct zst_layout_t *layout, char *name, struct Array *data, struct Array *references, char * section_name) {
    struct linked_symbol * lsym = malloc(sizeof(struct linked_symbol));
    struct zst_symbol_t * symbol = NULL;
    void * section = NULL;
    if (!lsym) return NULL;
    
    symbol = &lsym->symbol;
    symbol->name = name;
    symbol->content = 0;
    symbol->reference_count = array_size(references);
    symbol->references = 0;
    symbol->size = array_size(data);
    symbol->start_addr = 0;

    lsym->content = data;
    section = map_getkey_s(layout->section_map, section_name)->second;
    if (!section) {
        free(lsym);
        return NULL;
    }

    lsym->parent = section;
    lsym->references = references;
    map_adds_key(layout->symbol_map, name, lsym);
    return symbol;
}
