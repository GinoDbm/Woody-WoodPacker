#include "umbrella.h"
#include "elf_inject.h"




int main(void)
{
    Elf64_Phdr *seg = find_exec_segment(map, filesize);
    if (!seg)
    {
        fprintf(stderr, "Executable segment not found\n");
        return 1;
    }

    printf("EXEC SEGMENT:\n");
    printf("Offset: 0x%lx\n", seg->p_offset);
    printf("Vaddr : 0x%lx\n", seg->p_vaddr);
    printf("Filesz: 0x%lx\n", seg->p_filesz);
}
