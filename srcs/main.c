#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#include "../includes/umbrella.h"
#include "../includes/encrypt.h"

/* =========================
 * HEXDUMP
 * ========================= */
static void hexdump(const void *data, size_t size, size_t max)
{
    const unsigned char *p = data;
    size_t n = size < max ? size : max;

    for (size_t i = 0; i < n; i++)
    {
        printf("%02x ", p[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
}

int main(int ac, char **av)
{
    if (ac != 2)
    {
        fprintf(stderr, "Usage: %s <elf_binary>\n", av[0]);
        return 1;
    }

    void    *map;
    size_t  mapsize;
    int     fd;
    ElfInfo info;

    init_info(&info);

    /* ================================
     * 1. mmap
     * ================================ */
    map = map_file(av[1], &mapsize, &fd);
    if (!map)
        return 1;

    printf("[+] File mapped at %p (%zu bytes)\n", map, mapsize);

    /* ================================
     * 2. Validation ELF64
     * ================================ */
    if (validate_elf64(map, mapsize) < 0)
        return 1;

    printf("[+] ELF64 validated\n");

    /* ================================
     * 3. Parsing ELF
     * ================================ */
    if (parse_elf64(map, mapsize, &info) < 0)
        return 1;

    printf("[+] ELF parsed\n");
    printf("    Entry point : 0x%lx\n", info.entry);

    /* ================================
     * 4. Segment exécutable
     * ================================ */
    Elf64_Phdr *exec_ph = find_exec_segment(map, mapsize);
    if (!exec_ph)
    {
        fprintf(stderr, "[-] No executable segment found\n");
        return 1;
    }

    printf("[+] Exec segment found\n");
    printf("    Offset : 0x%lx\n", exec_ph->p_offset);
    printf("    Size   : %lu\n", exec_ph->p_filesz);
    printf("    Vaddr  : 0x%lx\n", exec_ph->p_vaddr);

    unsigned char *text_ptr =
        (unsigned char *)map + exec_ph->p_offset;
    size_t text_size = exec_ph->p_filesz;

    /* ================================
     * 5. Backup original
     * ================================ */
    unsigned char *backup = malloc(text_size);
    if (!backup)
    {
        perror("malloc");
        return 1;
    }

    memcpy(backup, text_ptr, text_size);
    printf("[+] Original text backed up\n");

    printf("\n[>] ORIGINAL (first 64 bytes)\n");
    hexdump(text_ptr, text_size, 64);

    /* ================================
     * 6. Clé XTEA
     * ================================ */
    uint32_t key[4] = {
        0x13371337,
        0xdeadbeef,
        0xcafebabe,
        0x42424242
    };

    /* ================================
     * 7. mprotect RW
     * ================================ */
    long pagesize = sysconf(_SC_PAGESIZE);
    uintptr_t start = (uintptr_t)text_ptr & ~(pagesize - 1);
    uintptr_t end = ((uintptr_t)text_ptr + text_size + pagesize - 1)
                    & ~(pagesize - 1);
    size_t len = end - start;

    if (mprotect((void *)start, len, PROT_READ | PROT_WRITE) == -1)
    {
        perror("mprotect RW");
        free(backup);
        munmap(map, mapsize);
        close(fd);
        return 1;
    }

    /* ================================
     * 8. CHIFFREMENT
     * ================================ */
    encrypt_text_xtea(text_ptr, text_size, key);
    printf("[+] Text encrypted\n");

    printf("\n[>] ENCRYPTED (first 64 bytes)\n");
    hexdump(text_ptr, text_size, 64);

    /* ================================
     * 9. DÉCHIFFREMENT
     * ================================ */
    decrypt_text_xtea(text_ptr, text_size, key);
    printf("[+] Text decrypted\n");

    printf("\n[>] DECRYPTED (first 64 bytes)\n");
    hexdump(text_ptr, text_size, 64);

    /* ================================
     * 10. Vérification
     * ================================ */
    if (memcmp(text_ptr, backup, text_size) == 0)
        printf("\n✅ SUCCESS: decrypted text matches original\n");
    else
        printf("\n❌ ERROR: decrypted text does NOT match original\n");

    /* ================================
     * 11. Remettre en lecture seule
     * ================================ */
    if (mprotect((void *)start, len, PROT_READ) == -1)
        perror("mprotect RO");

    /* ================================
     * 12. Cleanup
     * ================================ */
    free(backup);
    munmap(map, mapsize);
    close(fd);

    return 0;
}
