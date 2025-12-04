#include "../includes/umbrella.h"
#include "../includes/elf_inject.h"
#include "../includes/elf_parser64.h"

int main(int ac, char **av)
{
    void        *map;        // <-- ICI map est déclaré
    size_t      filesize;   // <-- ICI filesize est déclaré
    int         fd;
    Elf64_Phdr  *seg;

    if (ac != 2)
    {
        fprintf(stderr, "Usage: %s <binary>\n", av[0]);
        return 1;
    }

    map = map_file(av[1], &filesize, &fd);
    if (!map)
        return 1;

    if (validate_elf64(map, filesize) != 0)
    {
        munmap(map, filesize);
        close(fd);
        return 1;
    }

    seg = find_exec_segment(map, filesize);
    if (!seg)
    {
        fprintf(stderr, "Executable segment not found\n");
        munmap(map, filesize);
        close(fd);
        return 1;
    }

    printf("EXEC SEGMENT FOUND:\n");
    printf("Offset : 0x%lx\n", seg->p_offset);
    printf("Vaddr  : 0x%lx\n", seg->p_vaddr);
    printf("Filesz : 0x%lx\n", seg->p_filesz);
    printf("Memsz  : 0x%lx\n", seg->p_memsz);
    printf("Flags  : 0x%x\n", seg->p_flags);

    munmap(map, filesize);
    close(fd);
    return 0;
}
