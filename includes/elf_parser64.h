#pragma once
#include "umbrella.h"

typedef struct {
    int is64;
    int is_pie;               // Flag for DYN type (PIE file)
    uint64_t entry;           // Entrypoint (virtual address)
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

int elf_parse(const char *path, ElfInfo *out);
