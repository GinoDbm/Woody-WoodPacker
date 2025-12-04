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

void *get_exec_segment_ptr(void *map, ElfInfo *info)
{
    if (!info || !map)
        return NULL;
    if (info->seg_filesz == 0)
        return NULL;
    
    return (char *)map + info->seg_offset;
}

unsigned char *save_original_text(unsigned char *text, size_t size)
{
    unsigned char *save;

    if(!text || !size)
        return NULL;
    
    save = malloc(size);
    if (!save)
        return NULL;
    
    memcpy(save, text, size);
    return (save);
}