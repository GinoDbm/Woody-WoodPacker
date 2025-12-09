#include "stub.h"

/* Values patched by the packer */
uint64_t g_old_entry    = MAGIC_OLD_ENTRY;    // VA offset, not absolute address
uint64_t g_payload_off  = MAGIC_PAYLOAD_ADDR; // offset relative to ELF base
size_t   g_payload_size = MAGIC_PAYLOAD_SIZE;

/* Simple XOR decrypt */
void decrypt(uint8_t *buf, size_t size)
{
    uint64_t key = 0x4242424242424242ULL;

    for (size_t i = 0; i < size; i++)
        buf[i] ^= ((uint8_t *)&key)[i & 7];
}

__attribute__((naked))
void _start(void)
{
    __asm__ volatile(
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

        "jmp *%rbx\n"
        "0:\n"
    );
}