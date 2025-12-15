#include "../includes/umbrella.h"

void patch_stub(unsigned char *stub, size_t stub_size, uint64_t payload_addr, uint64_t payload_size, uint64_t original_entry);
void encrypt_text_xtea(unsigned char *text, size_t size, const uint32_t key[4]);

// Binary STUB created by : ld -r -b binary stub.bin
extern unsigned char _binary_stub_stub_bin_start[];
extern unsigned char _binary_stub_stub_bin_end[];

// XTEA 128-bit key (4 x 32-bit)
static const uint32_t g_xtea_key[4] = {0x42424242, 0x42424242, 0x42424242, 0x42424242};

int main(int ac, char **av)
{
    if (ac != 2) {
        fprintf(stderr, "Usage: %s <elf>\n", av[0]);
        return 1;
    }

    /* ---------------- Open file (read-only) ---------------- */
    int fd = open(av[1], O_RDONLY);
    if (fd < 0) 
    {
         perror("open");
        return 1; 
    }

    struct stat st;
    if (fstat(fd, &st) < 0)
    {
        perror("fstat");
        close(fd);
        return 1;
    }

    /* Map file read-only to get the content */
    uint8_t *file_map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_map == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    }

    /* Calculate stub size for total allocation */
    size_t stub_size_early = (size_t)(_binary_stub_stub_bin_end - _binary_stub_stub_bin_start);
    size_t final_size = st.st_size + stub_size_early;

    /* Allocate writable buffer for the output */
    uint8_t *map = mmap(NULL, final_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) 
    {
        perror("mmap output");
        return 1; 
    }

    /* Copy original file content to output buffer */
    memcpy(map, file_map, st.st_size);
    munmap(file_map, st.st_size);
    close(fd);

    /* Validate ELF header bounds */
    if ((size_t)st.st_size < sizeof(Elf64_Ehdr))
    {
        fprintf(stderr, "File too small for ELF header\n");
        munmap(map, final_size);
        return 1;
    }

    Elf64_Ehdr *eh = (Elf64_Ehdr *)map;

    /* Validate program header table bounds */
    if (eh->e_phoff > (size_t)st.st_size ||
        (eh->e_phnum > 0 && eh->e_phoff + (size_t)eh->e_phnum * sizeof(Elf64_Phdr) > (size_t)st.st_size))
    {
        fprintf(stderr, "Invalid ELF: program headers out of bounds\n");
        munmap(map, final_size);
        return 1;
    }

    Elf64_Phdr *ph = (Elf64_Phdr *)(map + eh->e_phoff);

    /*--------- Find segment PT_LOAD RX ---------- */
    Elf64_Phdr *text_seg = NULL;
    for (int i = 0; i < eh->e_phnum; i++)
    {
        if (ph[i].p_type == PT_LOAD && (ph[i].p_flags & PF_X))
        {
            text_seg = &ph[i];
            break;
        }
    }
    if (!text_seg)
    {
        fprintf(stderr, "No PT_LOAD executable segment\n");
        munmap(map, final_size);
        return 1;
    }

    /* ---------- Payload = .text = segment RX ---------- */
    uint64_t payload_offset = text_seg->p_offset;
    uint64_t payload_size   = text_seg->p_filesz;
    uint64_t payload_addr   = text_seg->p_vaddr;

    /* Validate text segment bounds */
    if (payload_offset > (size_t)st.st_size ||
        payload_offset + payload_size > (size_t)st.st_size)
    {
        fprintf(stderr, "Invalid ELF: text segment out of bounds\n");
        munmap(map, final_size);
        return 1;
    }

    unsigned char *payload = map + payload_offset;

    encrypt_text_xtea(payload, payload_size, g_xtea_key);

    uint64_t original_entry = eh->e_entry;

    /* ---------- STUB setup ---------- */
    size_t stub_size = (size_t)(_binary_stub_stub_bin_end - _binary_stub_stub_bin_start);
    unsigned char *stub_data = _binary_stub_stub_bin_start;

    uint64_t stub_file_offset = text_seg->p_offset + text_seg->p_filesz;
    uint64_t stub_vaddr       = text_seg->p_vaddr + text_seg->p_filesz;

    /* Validate stub injection bounds */
    if (stub_file_offset + stub_size > final_size)
    {
        fprintf(stderr, "Invalid ELF: not enough space for stub\n");
        munmap(map, final_size);
        return 1;
    }

    /* ---- Increase size of RX segment ---- */
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
    patch_stub(map + stub_file_offset, stub_size, payload_addr - aligned_stub_page, payload_size, original_entry - aligned_stub_page);

/* ---- Redirect entry ---- */
/* stub.bin layout with XTEA: .data (48 bytes) + decrypt + _start at offset 0x2de */
    #define STUB_START_OFFSET 0x2de
    eh = (Elf64_Ehdr *)map;
    eh->e_entry = stub_vaddr + STUB_START_OFFSET;

    /* ---- Write output file ---- */
    int out = open("woody", O_CREAT | O_WRONLY | O_TRUNC, 0755);
    if (out < 0) 
    {
         perror("open output");
        return 1; 
    }

    size_t output_size = st.st_size + stub_size;
    if (write(out, map, output_size) != (ssize_t)output_size) 
    {
        perror("write");
        close(out);
        return 1;
    }

    close(out);
    munmap(map, final_size);
    printf("Woody generated !\n");
    return 0;
}