#include "../includes/umbrella.h"
#include "../includes/encrypt.h"

/*
** Patch les trois placeholders du stub
*/
void patch_stub(unsigned char *stub, size_t stub_size,
                uint64_t payload_addr,
                uint64_t payload_size,
                uint64_t original_entry)
{
    for (size_t i = 0; i + 8 <= stub_size; i++)
    {
        uint64_t *p = (uint64_t *)(stub + i);

        if (*p == 0x1111111111111111ULL)
            *p = payload_addr;
        else if (*p == 0x2222222222222222ULL)
            *p = payload_size;
        else if (*p == 0x3333333333333333ULL)
            *p = original_entry;
    }
}

/*
** Symboles générés par ld -r -b binary sur : stub/stub.bin
** => _binary_stub_stub_bin_start / _binary_stub_stub_bin_end / _binary_stub_stub_bin_size
*/
extern unsigned char _binary_stub_stub_bin_start[];
extern unsigned char _binary_stub_stub_bin_end[];

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <ELF64>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = "woody";

    void *map = NULL;
    size_t mapsize = 0;
    int fd = -1;

    ElfInfo info;
    init_info(&info);

    /* mmap du fichier */
    map = map_file(input_path, &mapsize, &fd);
    if (!map)
        return 1;

    /* Valid ELF64 ? */
    if (validate_elf64(map, mapsize) < 0)
    {
        munmap(map, mapsize);
        close(fd);
        return 1;
    }

    /* Parse segments */
    if (parse_elf64(map, mapsize, &info) < 0)
    {
        munmap(map, mapsize);
        close(fd);
        return 1;
    }

    /* Déjà infecté ? */
    if (already_infected(map, &info))
    {
        fprintf(stderr, "File already infected.\n");
        munmap(map, mapsize);
        close(fd);
        return 1;
    }

    /* Localiser le segment exécutable */
    unsigned char *exec_segment = get_exec_segment_ptr(map, &info);
    if (!exec_segment)
    {
        fprintf(stderr, "Could not locate executable segment.\n");
        munmap(map, mapsize);
        close(fd);
        return 1;
    }

    /* Chiffrer le TEXT segment */
    const uint32_t key[4] = {0x42, 0x42, 0x42, 0x42};
    encrypt_text_xtea(exec_segment, info.seg_filesz, key);

    /*
    ** Charger le stub (données binaires linkées)
    */
    size_t stub_size = (size_t)(_binary_stub_stub_bin_end - _binary_stub_stub_bin_start);

    /* Très important : faire une copie modifiable */
    unsigned char *stub = malloc(stub_size);
    if (!stub)
    {
        fprintf(stderr, "malloc failed for stub\n");
        munmap(map, mapsize);
        close(fd);
        return 1;
    }

    memcpy(stub, _binary_stub_stub_bin_start, stub_size);

    uint64_t old_entry = info.entry;

    /* Patch du stub dans la copie */
    patch_stub(
        stub,
        stub_size,
        info.seg_vaddr,   // adresse du payload
        info.seg_filesz,  // taille
        old_entry         // ancien entrypoint
    );

    /* Injection */
    if (inject_payload(map, mapsize, &info, stub, stub_size, &old_entry) < 0)
    {
        fprintf(stderr, "Injection failed.\n");
        free(stub);
        munmap(map, mapsize);
        close(fd);
        return 1;
    }

    /* Écriture du fichier infecté */
    if (write_infected_file(output_path, map, mapsize) < 0)
    {
        fprintf(stderr, "Write failed.\n");
        free(stub);
        munmap(map, mapsize);
        close(fd);
        return 1;
    }

    free(stub);
    munmap(map, mapsize);
    close(fd);

    printf("WOODY WOOD PACKER\n");
    return 0;
}
