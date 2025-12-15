#include "stub.h"

/* Values patched by the packer */
uint64_t g_old_entry    = MAGIC_OLD_ENTRY;    // VA offset, not absolute address
uint64_t g_payload_off  = MAGIC_PAYLOAD_ADDR; // offset relative to ELF base
size_t   g_payload_size = MAGIC_PAYLOAD_SIZE;

/* WOODY message (14 bytes: 13 chars + newline) */
char g_woody_msg[16] = "....WOODY....\n";

/* XTEA constants */
#define XTEA_ROUNDS 32
#define XTEA_DELTA  0x9E3779B9

/* XTEA decrypt entire buffer - key is hardcoded to avoid global access issues */
void decrypt(uint8_t *buf, size_t size)
{
    /* XTEA 128-bit key (must match the key in main.c) */
    const uint32_t k[4] = {0x42424242, 0x42424242, 0x42424242, 0x42424242};

    size_t blocks = size / 8;

    for (size_t b = 0; b < blocks; b++)
    {
        /* Read 8 bytes as two uint32_t */
        uint32_t v0 = buf[b*8+0] | (buf[b*8+1] << 8) | (buf[b*8+2] << 16) | (buf[b*8+3] << 24);
        uint32_t v1 = buf[b*8+4] | (buf[b*8+5] << 8) | (buf[b*8+6] << 16) | (buf[b*8+7] << 24);

        /* XTEA decrypt block */
        uint32_t sum = XTEA_ROUNDS * XTEA_DELTA;
        for (int i = 0; i < XTEA_ROUNDS; i++)
        {
            v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
            sum -= XTEA_DELTA;
            v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
        }

        /* Write back 8 bytes */
        buf[b*8+0] = v0 & 0xFF;
        buf[b*8+1] = (v0 >> 8) & 0xFF;
        buf[b*8+2] = (v0 >> 16) & 0xFF;
        buf[b*8+3] = (v0 >> 24) & 0xFF;
        buf[b*8+4] = v1 & 0xFF;
        buf[b*8+5] = (v1 >> 8) & 0xFF;
        buf[b*8+6] = (v1 >> 16) & 0xFF;
        buf[b*8+7] = (v1 >> 24) & 0xFF;
    }
}

__attribute__((naked))
void _start(void)
{
    __asm__ volatile(
        /* Save all registers that the original _start expects to be preserved
           RDX contains rtld_fini pointer from dynamic linker
           RSP points to argc/argv/envp on stack */
        "push %rdx\n"
        "push %rdi\n"
        "push %rsi\n"
        "push %rcx\n"
        "push %r8\n"
        "push %r9\n"

        /* get current RIP */
        "lea 0f(%rip), %rbx\n"

        /* align down to page -> real ELF load base */
        "and $~0xfff, %rbx\n"

        /* rdi = real payload addr = base + offset */
        "lea g_payload_off(%rip), %rax\n"
        "mov (%rax), %rax\n"
        "add %rax, %rbx\n"
        "mov %rbx, %rdi\n"

        /* rsi = payload_size */
        "lea g_payload_size(%rip), %rsi\n"
        "mov (%rsi), %rsi\n"

        /* decrypt */
        "call decrypt\n"

        /* compute real old entry = base + old_offset */
        "lea 0f(%rip), %rbx\n"
        "and $~0xfff, %rbx\n"
        "lea g_old_entry(%rip), %rax\n"
        "mov (%rax), %rax\n"
        "add %rax, %rbx\n"

        /* Print "....WOODY....\n" using write syscall */
        "mov $1, %rax\n"                /* syscall: write */
        "mov $1, %rdi\n"                /* fd: stdout */
        "lea g_woody_msg(%rip), %rsi\n" /* buf: message */
        "mov $14, %rdx\n"               /* count: 14 bytes */
        "syscall\n"

        /* Restore registers */
        "pop %r9\n"
        "pop %r8\n"
        "pop %rcx\n"
        "pop %rsi\n"
        "pop %rdi\n"
        "pop %rdx\n"

        /* Jump to original entry */
        "jmp *%rbx\n"
        "0:\n"
    );
}
