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

    /* ---------------- Open file (read-only) ---------------- */
    int fd = open(av[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    printf("J'ai open mon fichier\n");

    struct stat st;
    if (fstat(fd, &st) < 0) { perror("fstat"); return 1; }

    /* Map file read-only to get the content */
    uint8_t *file_map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_map == MAP_FAILED) { perror("mmap"); return 1; }

    /* Calculate stub size for total allocation */
    size_t stub_size_early = (size_t)(_binary_stub_stub_bin_end
                              - _binary_stub_stub_bin_start);
    size_t final_size = st.st_size + stub_size_early;

    /* Allocate writable buffer for the output */
    uint8_t *map = mmap(NULL, final_size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) { perror("mmap output"); return 1; }

    /* Copy original file content to output buffer */
    memcpy(map, file_map, st.st_size);
    munmap(file_map, st.st_size);
    close(fd);
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

    /* ---- Add WRITE permission for decryption ---- */
    /* The stub needs to write to the .text segment to decrypt it */
    text_seg->p_flags |= PF_W;

    /* ---- Copy stub ---- */
    memcpy(map + stub_file_offset, stub_data, stub_size);

    /* ---- Patch the stub ---- */
    /* The stub computes base = RIP & ~0xfff, so offsets must be relative
       to that aligned address, not to the actual ELF base (0 for PIE) */
    uint64_t aligned_stub_page = stub_vaddr & ~0xFFFULL;
    patch_stub(map + stub_file_offset, stub_size,
               payload_addr - aligned_stub_page,
               payload_size,
               original_entry - aligned_stub_page);

    printf("[DEBUG] stub_vaddr = 0x%lx\n", stub_vaddr);
    printf("[DEBUG] aligned_stub_page = 0x%lx\n", aligned_stub_page);
    printf("[DEBUG] payload_addr = 0x%lx\n", payload_addr);
    printf("[DEBUG] payload_size = %lu\n", payload_size);
    printf("[DEBUG] old_entry    = 0x%lx\n", original_entry);
    printf("[DEBUG] patched g_payload_off = 0x%lx (signed: %ld)\n",
           payload_addr - aligned_stub_page, (int64_t)(payload_addr - aligned_stub_page));
    printf("[DEBUG] patched g_old_entry = 0x%lx (signed: %ld)\n",
           original_entry - aligned_stub_page, (int64_t)(original_entry - aligned_stub_page));

    /* ---- Redirect entry ---- */
    /* stub.bin layout: .data (24 bytes) + decrypt (0x6e bytes) + _start
       So _start is at offset 0x18 + 0x6e = 0x86 in stub.bin */
    #define STUB_START_OFFSET 0x86
    eh = (Elf64_Ehdr *)map;
    eh->e_entry = stub_vaddr + STUB_START_OFFSET;

    /* ---- Write output file ---- */
    int out = open("woody", O_CREAT | O_WRONLY | O_TRUNC, 0755);
    if (out < 0) { perror("open output"); return 1; }

    size_t output_size = st.st_size + stub_size;
    if (write(out, map, output_size) != (ssize_t)output_size) {
        perror("write");
        close(out);
        return 1;
    }

    close(out);
    munmap(map, final_size);
    printf("✔ woody generated\n");
    return 0;
}