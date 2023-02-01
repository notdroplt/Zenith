#include <elfcreator.h>

#include <stdlib.h>
#include <string.h>

static void setup_file_header(Elf64_Ehdr * header) {
    header->e_ident[EI_MAG0] = ELFMAG0;
    header->e_ident[EI_MAG1] = ELFMAG1;
    header->e_ident[EI_MAG2] = ELFMAG2;
    header->e_ident[EI_MAG3] = ELFMAG3;
    header->e_ident[EI_CLASS] = ELFCLASS64;
    header->e_ident[EI_DATA] = ELFDATA2LSB;
    header->e_ident[EI_VERSION] = EV_CURRENT;
    header->e_ident[EI_OSABI] = ELFOSABI_SYSV;
    header->e_ident[EI_ABIVERSION] = 0;

    header->e_type = ET_REL;
    header->e_machine = EM_NONE; /* will change only if we get a EM_ZENITHVM somehow*/
    header->e_version = EV_CURRENT;

    header->e_entry = 0x0000;
    header->e_phoff = sizeof(Elf64_Ehdr); // the struct is already aligned
    //e_shoff
    //e_flags

    header->e_ehsize = sizeof(Elf64_Ehdr);
    header->e_phentsize = sizeof(Elf64_Phdr);
    header->e_phnum = 2; // (pheader), (code and)
    header->e_shentsize = sizeof(Elf64_Shdr);
    //e_shnum
    //e_shstrndx
}

void generate_pheaders(Elf64_Phdr *program_headers) {
    Elf64_Phdr * self_header = &program_headers[0];
    Elf64_Phdr * code_header = &program_headers[1];

    self_header->p_type = PT_PHDR; 
    self_header->p_flags = 0x00;
    self_header->p_offset = sizeof(Elf64_Ehdr);
    self_header->p_vaddr = sizeof(Elf64_Ehdr);
    self_header->p_paddr = sizeof(Elf64_Ehdr);
    self_header->p_filesz = sizeof(Elf64_Ehdr) * 4;
    self_header->p_memsz = sizeof(Elf64_Ehdr) * 4;
    self_header->p_align = 0x8;

    ///TODO: make data, rodata, bss and text at different program headers
    code_header->p_type = PT_LOAD;
    code_header->p_flags = PF_X | PF_W | PF_R;
    code_header->p_offset = sizeof(Elf64_Ehdr) + sizeof(Elf64_Ehdr) * 4;
    code_header->p_vaddr = 0x10000;
    code_header->p_paddr = 0x10000;
    code_header->p_filesz = 0;
    code_header->p_memsz = 0;
    code_header->p_align = 0x10;
}


struct elf_layout_t * create_elf(){
    struct elf_layout_t * layout;
    layout = malloc(sizeof(struct elf_layout_t));
    layout->section_headers = malloc(sizeof(Elf64_Shdr) * 4);
    layout->section_counter = 0;
    layout->section_strings = malloc(1);
    layout->strings_size = 0;
    if (!layout) return NULL;

    setup_file_header(&layout->header);
    generate_pheaders(&layout->program_headers[0]);
    generate_section(layout, ".text");
    generate_section(layout, ".data");
    generate_section(layout, ".rodata");
    generate_section(layout, ".bss");

    return layout;
}

static uint64_t add_name_to_shtab(struct elf_layout_t * layout, char * name) {
    void * tmp = realloc(layout->section_strings, layout->strings_size + strlen(name) + 1);
    if (!tmp) {
        free(layout->section_strings);
        return -1;
    }

    layout->section_strings = tmp;

    strcpy((char*)((uint64_t)layout->section_strings + layout->strings_size), name);
    *(char *)((uint64_t)layout->section_strings + layout->strings_size + 1) = 0;
    layout->strings_size += strlen(name);
    return layout->strings_size - strlen(name);
}

Elf64_Shdr * generate_section(struct elf_layout_t * layout, char * section_name){
    Elf64_Shdr * current = &layout->section_headers[layout->section_counter];
    layout->section_counter++;

    current->sh_name = add_name_to_shtab(layout, section_name);
    if (current->sh_name == (Elf64_Word)-1) return NULL;
    current->sh_entsize = 0;
    if (!section_name) {
        
    }
    else if (strcmp(section_name, ".text") == 0) {
        current->sh_type = SHT_PROGBITS;
        current->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    } else if (strcmp(section_name, ".symtab") == 0) {
        current->sh_type = SHT_SYMTAB;
        current->sh_flags = 0;
    } else if (strcmp(section_name, ".strtab") == 0 || strcmp(section_name, ".shstrtab") == 0) {
        current->sh_type = SHT_STRTAB;
        current->sh_flags = 0;
    } else {
        current->sh_type = SHT_PROGBITS;
        current->sh_flags = SHF_WRITE | SHF_ALLOC;
    }
    current->sh_offset = 0;
    current->sh_size = 0;
    current->sh_link = 0;
    current->sh_addralign = 0x10;

    return current;
}