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