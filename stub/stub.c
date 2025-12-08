__attribute__((section(".rodata")))
static const unsigned long g_key = 0x4242424242424242;

extern void _start(void);

static void decrypt(unsigned char *p, unsigned long size)
{
    for (unsigned long i = 0; i < size; i++)
        p[i] ^= ((unsigned char*)&g_key)[i % 8];
}

__attribute__((section(".text")))
void _start(void)
{
    // Adresse du segment à décrypter (patchée par ton packer)
    unsigned char *payload     = (unsigned char*)0x1111111111111111;
    unsigned long  payload_len = 0x2222222222222222;

    decrypt(payload, payload_len);

    // Saut vers l'entrypoint original (patché aussi par packer)
    void (*original_entry)(void) = (void(*)(void))0x3333333333333333;
    original_entry();
}
