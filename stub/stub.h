#ifndef STUB_H
# define STUB_H

#include <stdint.h>
#include <stddef.h>

#define MAGIC_PAYLOAD_ADDR 0xAAAAAAAAAAAAAAAAULL
#define MAGIC_PAYLOAD_SIZE 0xBBBBBBBBBBBBBBBBULL
#define MAGIC_OLD_ENTRY    0xCCCCCCCCCCCCCCCCULL

extern int64_t  g_old_entry;
extern int64_t  g_payload_off;
extern size_t   g_payload_size;

void decrypt(uint8_t *buf, size_t size);        // XTEA decrypts the encrypted payload
void _start(void) __attribute__((noreturn));    // Stub entry point: decrypts and jumps to original entry

#endif