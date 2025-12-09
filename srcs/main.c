#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>

void patch_stub(unsigned char *stub, size_t stub_size,
                uint64_t payload_addr, uint64_t payload_size,
                uint64_t original_entry);

// Stub binaire généré via ld -r -b binary stub.bin
extern unsigned char _binary_stub_stub_bin_start[];
extern unsigned char _binary_stub_stub_bin_end[];

static void encrypt(unsigned char *p, size_t size)
{
    const uint64_t key = 0x4242424242424242ULL;
    for (size_t i = 0; i < size; i++)
        p[i] ^= ((unsigned char *)&key)[i & 7];
}

int main(int ac, char **av)
{
    if (ac != 2) {
        fprintf(stderr, "Usage: %s <elf>\n", av[0]);
        return 1;
    }

    /* ---------------- Open file ---------------- */
    int fd = open(av[1], O_RDWR);
    if (fd < 0) { perror("open"); return 1; }
    printf("J'ai open mon fichier\n");

    struct stat st;
    if (fstat(fd, &st) < 0) { perror("fstat"); return 1; }

    uint8_t *map = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) { perror("mmap"); return 1; }
    printf("J'ai mmap ma map\n");

    Elf64_Ehdr *eh = (Elf64_Ehdr *)map;
    Elf64_Phdr *ph = (Elf64_Phdr *)(map + eh->e_phoff);

    printf("eh->e_phoff = %lu\n", (uint64_t)eh->e_phoff);
    printf("eh->e_phnum = %u\n", eh->e_phnum);
    printf("st.st_size = %lu\n", (uint64_t)st.st_size);

    /* ---------- Trouver segment PT_LOAD RX ---------- */
    Elf64_Phdr *text_seg = NULL;
    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type == PT_LOAD && (ph[i].p_flags & PF_X)) {
            text_seg = &ph[i];
            break;
        }
    }
    if (!text_seg) {
        fprintf(stderr, "No PT_LOAD executable segment\n");
        return 1;
    }

    /* ---------- Payload = .text = segment RX ---------- */
    uint64_t payload_offset = text_seg->p_offset;
    uint64_t payload_size   = text_seg->p_filesz;
    uint64_t payload_addr   = text_seg->p_vaddr;

    unsigned char *payload = map + payload_offset;

    encrypt(payload, payload_size);

    uint64_t original_entry = eh->e_entry;

    /* ---------- Préparation du STUB ---------- */
    size_t stub_size = (size_t)(_binary_stub_stub_bin_end
                              - _binary_stub_stub_bin_start);
    unsigned char *stub_data = _binary_stub_stub_bin_start;

    uint64_t stub_file_offset = text_seg->p_offset + text_seg->p_filesz;
    uint64_t stub_vaddr       = text_seg->p_vaddr + text_seg->p_filesz;

    /* ---- EXTENSION DU SEGMENT RX ---- */
    text_seg->p_filesz += stub_size;
    text_seg->p_memsz  += stub_size;

    /* ---- On étend le fichier AVANT de remapper ---- */
    if (ftruncate(fd, st.st_size + stub_size) < 0) {
        perror("ftruncate");
        return 1;
    }

    /* ---- NEW MMAP ---- */
    munmap(map, st.st_size);
    map = mmap(NULL, st.st_size + stub_size, PROT_READ | PROT_WRITE,
               MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) { perror("mmap 2"); return 1; }
    printf("j'ai mmap mon fichier\n");

    /* ---- Copy stub ---- */
    memcpy(map + stub_file_offset, stub_data, stub_size);

    /* ---- Patch the stub ---- */
    patch_stub(map + stub_file_offset, stub_size,
               payload_addr, payload_size, original_entry);

    printf("[DEBUG] stub_vaddr = 0x%lx\n", stub_vaddr);
    printf("[DEBUG] payload_addr = 0x%lx\n", payload_addr);
    printf("[DEBUG] payload_size = %lu\n", payload_size);
    printf("[DEBUG] old_entry    = 0x%lx\n", original_entry);
    printf("[DEBUG] g_payload_off = 0x%lx\n", stub_file_offset - text_seg->p_offset);
    printf("[DEBUG] g_old_entry_off = 0x%lx\n",
           original_entry - text_seg->p_vaddr);

    /* ---- Redirect entry ---- */
    eh = (Elf64_Ehdr *)map;
    eh->e_entry = stub_vaddr;

    /* ---- Write output file ---- */
    int out = open("woody", O_CREAT | O_WRONLY | O_TRUNC, 0755);
    if (out < 0) { perror("open output"); return 1; }

    size_t final_size = st.st_size + stub_size;
    if (write(out, map, final_size) != (ssize_t)final_size) {
        perror("write");
        close(out);
        return 1;
    }

    close(out);
    close(fd);
    printf("✔ woody generated\n");
    return 0;
}
