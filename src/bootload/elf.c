#include "defines.h"
#include "elf.h"
#include "lib.h"

struct elf_header {
    struct {
        unsigned char magic[4]; // magic number
        unsigned char class; // 32/64 bit
        unsigned char format; // endian
        unsigned char version; // elf format version
        unsigned char abi; // OS type
        unsigned char abi_version; // OS version
        unsigned char reserve[7]; // not used
    } id; // 16byte identification information

    short type; // file type
    short arch; // CPU type
    long version; // elf format version
    long entry_point;
    long program_header_offset;
    long section_header_offset;
    long flags;
    short header_size;
    short program_header_size;
    short program_header_num;
    short section_header_size;
    short section_header_num;
    short section_name_index; // index of section containing section names
};

struct elf_program_header {
    long type; // segment type
    long offset; // offset in whole file
    long virtual_addr; // VA
    long physical_addr; // PA
    long file_size; // segment size in file
    long memory_size; // segment size on memory
    long flags;
    long align;
};

static int elf_check(struct elf_header *header) {
    if (memcmp(header->id.magic, "\x7f" "ELF", 4)) // check magic number
        return -1;

    if (header->id.class != 1) return -1; // ELF32
    if (header->id.format != 2) return -1; // big endian
    if (header->id.version != 1) return -1; // version 1
    if (header->type != 2) return -1; // Executable file
    if (header->version != 1) return -1; // version 1

    // Hitachi H8/300 or H8/300H
    if ((header->arch != 46) && (header->arch != 47)) return -1;

    return 0;
}

static int elf_load_program(struct elf_header *header) {
    int i;
    struct elf_program_header *phdr;

    for (i = 0; i < header->program_header_num; i++) {
        // get program header
        phdr = (struct elf_program_header *)
                ((char *) header + header->program_header_offset +
                 header->program_header_size * i);

        // check whether loadable segment or not
        if (phdr->type != 1)
            continue;

        memcpy((char *) phdr->physical_addr, (char *) header + phdr->offset,
               phdr->file_size);
        memset((char *) phdr->physical_addr + phdr->file_size, 0,
               phdr->memory_size - phdr->file_size);
    }

    return 0;
}

char *elf_load(char *buf) {
    struct elf_header *header = (struct elf_header *) buf;

    if (elf_check(header) < 0) return NULL;
    if (elf_load_program(header)) return NULL;
    return (char *) header->entry_point;
}

