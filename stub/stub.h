#ifndef STUB_H
# define STUB_H

#include <stdint.h>
#include <stddef.h>

#define MAGIC_PAYLOAD_ADDR 0xAAAAAAAAAAAAAAAAULL
#define MAGIC_PAYLOAD_SIZE 0xBBBBBBBBBBBBBBBBULL
#define MAGIC_OLD_ENTRY    0xCCCCCCCCCCCCCCCCULL

extern uint64_t g_old_entry;
extern uint8_t *g_payload_addr;
extern size_t   g_payload_size;

void decrypt(uint8_t *buf, size_t size);        // XOR decrypts the encrypted payload
void _start(void) __attribute__((noreturn));    // Stub entry point: decrypts and jumps to original entry

#endif