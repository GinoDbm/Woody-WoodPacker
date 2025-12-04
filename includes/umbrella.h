#ifndef PROJECT_H
#define PROJECT_H

/* C Headers */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

/* POSIX */
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/auxv.h>

/* ELF definitions */
#include <elf.h>

/* Personal headers */
#include "elf_parser64.h"
#include "elf_inject.h"
#include "encrypt.h"
//#include "woody.h"
//#include "woody_generic.h"
//#include "woody_elf.h"     /* ELF-specific declarations */
//#include "crypto.h"        /* Encryption header */
//#include "injector.h"      /* Injector header */

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096UL
#endif

#endif
