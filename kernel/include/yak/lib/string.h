#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

int isdigit(int c);
int isprint(int c);

size_t strlen(const char *str);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);

void *memset(void *ptr, int value, size_t n);
void *memcpy(void * restrict dst, const void * restrict src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
int memcmp(const void* aptr, const void* bptr, size_t size);

#endif
