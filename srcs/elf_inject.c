#include "umbrella.h"

Elf64_Phdr *find_exec_segment(void *map, size_t mapsize)
{
    Elf64_Ehdr *eh;
    Elf64_Phdr *ph;
    size_t i;

    if (!map || mapsize < sizeof(Elf64_Ehdr))
        return NULL;

    eh = (Elf64_Ehdr *)map;

    if (eh->e_phoff + eh->e_phnum * sizeof(Elf64_Phdr) > mapsize)
        return NULL;

    ph = (Elf64_Phdr *)((char *)map + eh->e_phoff);

    for (i = 0; i < eh->e_phnum; i++)
    {
        if (ph[i].p_type == PT_LOAD && (ph[i].p_flags & PF_X))
            return &ph[i];
    }

    return NULL;
}