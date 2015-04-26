#include <yak/lib/types.h>
#include <yak/lib/string.h>

int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

int isprint(int c)
{
    return c > 0x1f && c != 0x7f;
}

size_t strlen(const char *str)
{
    size_t ret = 0;
    while (str[ret]) ++ret;
    return ret;
}

char *strcpy(char *dst, const char *src)
{
    while ((*dst++ = *src++)) ;
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    while (n-- && (*dst++ = *src++)) ;
    for (; n--; *dst++ = '\0') ;
    return dst;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    for (; n > 0; s1++, s2++, --n)
        if (*s1 != *s2)
            return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
        else if (*s1 == '\0')
            return 0;
    return 0;
}

void *memset(void *ptr, int value, size_t n)
{
    if (value != 0 || ((uintptr_t)ptr & 7) != 0) {
        asm volatile ("rep stosb" :: "D"(ptr), "a"(value), "c"(n) : "memory");
    } else {
        unsigned char *dst = ptr;
        size_t qwords = n / 8;
        size_t trail_bytes = n % 8;

        asm volatile ("rep stosq" : "=D"(dst) : "D"(dst), "a"(0), "c"(qwords) : "memory");
        while (trail_bytes--)
            *dst++ = 0;
    }
    return ptr;
}

void *memcpy(void *restrict dst, const void *restrict src, size_t n)
{
    uintptr_t d = (uintptr_t)dst;
    uintptr_t s = (uintptr_t)src;

    if ((d & 7) != (s & 7)) {
        asm volatile("rep movsb" : : "D"(d), "S"(s), "c"(n) : "memory");
    } else {
        while ((s & 7) && n) {
            *(char *)d++ = *(char *)s++;
            --n;
        }
        asm volatile ("rep movsq" : "=D"(d), "=S"(s) : "0"(d), "1"(s), "c"(n / 8) : "memory");
        n &= 7;
        while (n--)
            *(char *)d++ = *(char *)s++;
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    char *d = (char *)dst;
    const char *s = (const char *)src;
    char *retval = d;

    if (n == 0 || dst == src)
        return dst;

    if (d > s + n)
        return memcpy(d, s, n);
    if (d + n < s)
        return memcpy(d, s, n);
    if (d < s)
        return memcpy(d, s, n);

    return retval;
}

int memcmp(const void* aptr, const void* bptr, size_t size)
{
	const unsigned char* a = (const unsigned char*) aptr;
	const unsigned char* b = (const unsigned char*) bptr;

	for (size_t i = 0; i < size; ++i)
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
	return 0;
}
