#ifndef ENCRYPT_H
#define ENCRYPT_H

#pragma once
#include "umbrella.h"

#define XTEA_DELTA  0x9E3779B9
#define XTEA_ROUNDS 32

void xtea_encrypt_block(uint32_t v[2], const uint32_t k[4]);
void xtea_decrypt_block(uint32_t v[2], const uint32_t k[4]);

void encrypt_text_xtea(unsigned char *text, size_t size, const uint32_t key[4]);
void decrypt_text_xtea(unsigned char *text, size_t size, const uint32_t key[4]);

#endif
