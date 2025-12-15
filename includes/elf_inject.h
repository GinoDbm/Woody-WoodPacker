#ifndef ELF_INJECT_H
#define ELF_INJECT_H

#pragma once
#include "umbrella.h"

Elf64_Phdr *find_exec_segment(void *map, size_t mapsize); // Finds the executable PT_LOAD segment
void *get_exec_segment_ptr(void *map, ElfInfo *info); // Returns pointer to executable segment data

unsigned char *save_original_text(unsigned char *text, size_t size); // Saves a copy of the original .text section
int inject_payload(void *map, size_t mapsize, ElfInfo *info, unsigned char *payload, size_t payload_size, uint64_t *old_entry); // Injects payload into executable segment

int write_infected_file(const char *output_path, void *map, size_t size); // Writes the modified ELF to disk
int already_infected(void *map, ElfInfo *info); // Checks if the ELF is already packed
void patch_stub(unsigned char *stub, size_t stub_size, uint64_t payload_rel_off, uint64_t payload_size, uint64_t old_entry_rel_off); // Patches magic values in stub with actual offsets

#endif
