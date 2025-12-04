#include "stub.h"

uint64_t g_old_entry = MAGIC_OLD_ENTRY;
uint8_t  *g_text_addr = (uint8_t *)MAGIC_TEXT_ADDR;
size_t   g_text_size = MAGIC_TEXT_SIZE;

/* XTEA key*/
uint32_t g_key[4] = {0x13371337, 0x42424242, 0xdeadbeef, 0xcafebabe};

/*XTEA Decrypt*/

static void xtea_decrypt_block(uint32_t v[2], const uint32_t k[4])
{
    uint32_t v0 = v[0], v1 = v[1];
    uint32_t sum = XTEA_ROUNDS * XTEA_DELTA;

    for (int i = 0; i < XTEA_ROUNDS; i++)
    {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
        sum -= XTEA_DELTA;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
    }

    v[0] = v0;
    v[1] = v1;
}

static void decrypt_text(void)
{
    size_t blocks = g_text_size / 8;
    uint32_t *ptr = (uint32_t *)g_text_addr;

    for (size_t i = 0; i < blocks; i++)
        xtea_decrypt_block(&ptr[i * 2], g_key);
}

/*Stub Entrypoint*/

__attribute__((noreturn))
void _start(void)
{
    decrypt_text();

    /* Saut vers l'ancien entry point */
    void (*entry)(void) = (void (*)(void))g_old_entry;
    entry();

    __builtin_unreachable();
}
