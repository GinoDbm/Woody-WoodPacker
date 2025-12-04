#include "../includes/encrypt.h"

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

    for (int i = 0; i < XTEA_ROUNDS; i++)
    {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
        sum -= XTEA_DELTA;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
    }
    v[0] = v0;
    v[1] = v1;
 }

void encrypt_text_xtea(unsigned char *text, size_t size, const uint32_t key[4])
{
    size_t blocks = (size + 7) / 8; // arrondi vers le haut
    uint32_t block[2];

    for (size_t i = 0; i < blocks; i++)
    {
        block[0] = 0;
        block[1] = 0;
        for (size_t j = 0; j < 8; j++)
        {
            size_t idx = i * 8 + j;
            if (idx < size)
                ((unsigned char*)block)[j] = text[idx];
        }
        xtea_encrypt_block(block, key);

        for (size_t j = 0; j < 8; j++)
        {
            size_t idx = i * 8 + j;
            if (idx < size)
                text[idx] = ((unsigned char*)block)[j];
        }
    }
}


void decrypt_text_xtea(unsigned char *text, size_t size, const uint32_t key[4])
{
    size_t blocks = (size + 7) / 8;
    uint32_t block[2];

    for (size_t i = 0; i < blocks; i++)
    {
        
        block[0] = 0;
        block[1] = 0;

        for (size_t j = 0; j < 8; j++)
        {
            size_t idx = i * 8 + j;
            if (idx < size)
                ((unsigned char*)block)[j] = text[idx];
        }
        xtea_decrypt_block(block, key);
        
        for (size_t j = 0; j < 8; j++)
        {
            size_t idx = i * 8 + j;
            if (idx < size)
                text[idx] = ((unsigned char*)block)[j];
        }
    }
}
