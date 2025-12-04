#ifndef STUB_H
# define STUB_H

#pragma once
#include "umbrella.h"

# define XTEA_ROUNDS 32
# define XTEA_DELTA  0x9E3779B9

/* Marqueurs magiques (pour le patch binaire) */
# define MAGIC_OLD_ENTRY 0xAAAAAAAAAAAAAAAAULL
# define MAGIC_TEXT_ADDR 0xBBBBBBBBBBBBBBBBULL
# define MAGIC_TEXT_SIZE 0xCCCCCCCCCCCCCCCCULL

/* These values gonna be patch by the injector */
extern uint64_t g_old_entry;
extern uint8_t  *g_text_addr;
extern size_t   g_text_size;
extern uint32_t g_key[4];

/* Entrypoint of the stub */
void _start(void) __attribute__((noreturn));

#endif