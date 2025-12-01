#pragma once
#include "umbrella.h"

Elf64_Phdr *find_exec_segment(void *map, size_t mapsize);
