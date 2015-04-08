#ifndef __COMMON_H__
#define __COMMON_H__

#include <yak/lib/types.h>

void put_char(const char c);
void print(const char *str);
int strncmp(const char *s1, const char *s2, unsigned int n);
void* memset(void* bufptr, int value, u32_t size);

#ifdef _DEBUG_

void print_num(unsigned long long num, int size);
void show_mem(const char *start, int size, int nperline);

#endif

#endif
