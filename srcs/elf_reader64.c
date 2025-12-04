#include "../includes/umbrella.h"

/* Function to initialize our struct */
void init_info(ElfInfo *o)
{
    if(!o)
        return ;
    memset(o, 0, sizeof(*o));
    o->seg_align = 0;
}

/* Function that mmap the file (read-only) */
void *map_file(const char *path, size_t *size_out, int *out_fd)
{
    if (!path || !size_out)
        return NULL;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return NULL;
    }

    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1)
    {
        perror("lseek SEEK_END");
        close(fd);
        return NULL;
    }

    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        perror("lseek SEEK_SET");
        close(fd);
        return (NULL);
    }

    if (size == 0)
    {
        fprintf(stderr, "File is empty\n");
        close(fd);
        return NULL;
    }

    void *map = mmap(NULL, (size_t)size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED)
    {
        perror("mmap");
        close (fd);
        return NULL;
    }

    *size_out = (size_t)size;

    if (out_fd)
        *out_fd = fd;
    else
        close(fd);
    return (map);
}

/* Function that checks if the file is a ELF64 file. */
int validate_elf64(void *map, size_t mapsize)
{
    if (!map || mapsize < EI_NIDENT)
        return (-1);
    
    unsigned char *ident = (unsigned char *) map;
    if (ident[EI_MAG0] != ELFMAG0 || ident[EI_MAG1] != ELFMAG1 ||
        ident[EI_MAG2] != ELFMAG2 || ident[EI_MAG3] != ELFMAG3)
    {
        fprintf(stderr, "Not an ELF file\n");
        return (-1);
    }

    if (ident[EI_CLASS] != ELFCLASS64)
    {
        fprintf(stderr, "Not a 64-bit ELF file\n");
        return (-1);
    }

    return (0);
}

/* Function that fills our struct ElfInfo */
int parse_elf64(void *map, size_t mapsize, ElfInfo *o)
{
    if(!map || !o)  //Check if map or ElfInfo variable is empty
        return (-1);
    
    Elf64_Ehdr *eh = (Elf64_Ehdr *)map; //Casts the map to Elf64_Ehdr (ELF64 header type)

    if((size_t)eh->e_ehsize > sizeof(Elf64_Ehdr) || eh->e_phoff + eh->e_phnum * eh->e_phentsize > mapsize) //Protection again corrupted files, invalid read, offset overflow
    {
        fprintf(stderr, "Invalid ELF header or program headers out of bounds\n");
        return -1;
    }

    o->is64 = 1;
    o->entry = eh->e_entry;
    o->is_pie = (eh->e_type == ET_DYN) ? 1 : 0;

    Elf64_Phdr *ph =(Elf64_Phdr *)((char *)map + eh->e_phoff);
    int found = 0;
    for(int i=0;i<eh->e_phnum;++i)
    {
        if(ph[i].p_type == PT_LOAD && (ph[i].p_flags & PF_X)) //Checks if there's an executable segment to load
        {
            o->seg_offset = ph[i].p_offset;
            o->seg_vaddr  = ph[i].p_vaddr;
            o->seg_filesz = ph[i].p_filesz;
            o->seg_memsz  = ph[i].p_memsz;
            o->seg_flags  = ph[i].p_flags;
            o->seg_align  = ph[i].p_align;
            found = 1;
            break;
        }
    }

    if(!found)
    {
        fprintf(stderr, "No executable PT_LOAD segment found\n");
        return (-1);
    }
    return (0);
}