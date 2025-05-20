#ifndef STDMEM_H
#define STDMEM_H

#include <stddef.h>

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// These are just declarations. Implementations are in stdmem.c

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

#endif
