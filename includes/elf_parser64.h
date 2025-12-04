#pragma once
#include "umbrella.h"

typedef struct {
    int is64;
    int is_pie;               
    uint64_t entry;
    // executable segment (PT_LOAD with PF_X)
    uint64_t seg_offset;
    uint64_t seg_vaddr;
    uint64_t seg_filesz;
    uint64_t seg_memsz;
    uint32_t seg_flags;
    uint64_t seg_align;
    // .text section (if present)
    uint64_t text_sh_offset;
    uint64_t text_sh_addr;
    uint64_t text_sh_size;
    int has_text_section;
} ElfInfo;

void init_info(ElfInfo *o);
void *map_file(const char *path, size_t *size_out, int *out_fd);
int validate_elf64(void *map, size_t mapsize);
int parse_elf64(void *map, size_t mapsize, ElfInfo *o);