#include <stdint.h>
#include <stddef.h>
#include "libft.h"

#define XTEA_ROUNDS 32
#define XTEA_DELTA  0x9E3779B9

/* --- XTEA encrypt/decrypt 64 bits --- */
void xtea_encrypt_block(uint32_t v[2], const uint32_t k[4])
{
    uint32_t v0 = v[0], v1 = v[1];
    uint32_t sum = 0;

    for(int i = 0; i < XTEA_ROUNDS; i++)
    {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
        sum += XTEA_DELTA;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
    }
    v[0] = v0;
    v[1] = v1;
}

void xtea_decrypt_block(uint32_t v[2], const uint32_t k[4])
{
    uint32_t v0 = v[0], v1 = v[1];
    uint32_t sum = XTEA_ROUNDS * XTEA_DELTA;

    for(int i = 0; i < XTEA_ROUNDS; i++)
    {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
        sum -= XTEA_DELTA;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
    }
    v[0] = v0;
    v[1] = v1;
}

/* --- Encrypt entire buffer safely --- */
void encrypt_text_xtea(unsigned char *text, size_t size, const uint32_t key[4])
{
    size_t blocks = size / 8;   // 8 octets par bloc XTEA
    uint32_t v[2];

    for (size_t i = 0; i < blocks; i++)
    {
        ft_memcpy(v, text + i*8, 8);       // Secured copy
        xtea_encrypt_block(v, key);        // Encrypting the block
        ft_memcpy(text + i*8, v, 8);       // Write back
    }
}

/* --- Decrypt entire buffer safely --- */
void decrypt_text_xtea(unsigned char *text, size_t size, const uint32_t key[4])
{
    size_t blocks = size / 8;
    uint32_t v[2];

    for (size_t i = 0; i < blocks; i++)
    {
        ft_memcpy(v, text + i*8, 8);       // Secured copy
        xtea_decrypt_block(v, key);        // Decrypting the block
        ft_memcpy(text + i*8, v, 8);       // Write back
    }
}
