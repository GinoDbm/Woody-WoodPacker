#pragma once
#include "umbrella.h"

Elf64_Phdr *find_exec_segment(void *map, size_t mapsize);
unsigned char *save_original_text(unsigned char *text, size_t size);
void encrypt_text(unsigned char *text, size_t size, unsigned char key);
void decrypt_text(unsigned char *text, size_t size, unsigned char key);
int inject_elf(const char *path);

