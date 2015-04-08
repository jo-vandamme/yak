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
    asm volatile("cld; rep stosb"
                 : "+c"(n), "+D"(ptr)
                 : "a"(value)
                 : "memory");
    return ptr;
}

void *memcpy(void *restrict dst, const void *restrict src, size_t n)
{
    /*
     * memcpy does not support overlapping buffers, so always do it
     * forwards.
     * For speedy copying, optimize the common case where both pointers
     * and the length are word-aligned, and copy word-at-a-time instead
     * of byte-at-a-time. Otherwise, copy by bytes.
     */
    /*
    void *d = dst;
    if (((uintptr_t)dst | (uintptr_t)src | n) & 3) {
        __asm__ volatile("cld; rep movsb (%%esi), %%es:(%%edi)"
                         : "+c" (n), "+S" (src), "+D" (d)
                         : : "cc", "memory");
        return dst;
    }
    n /= 4;
    __asm__ volatile ("cld; rep movsl (%%esi), %%es:(%%edi)"
                      : "+c" (n), "+S" (src), "+D" (d)
                      : : "cc", "memory");
    return dst;
    */
    unsigned char *p = (unsigned char *)src;
    unsigned char *q = (unsigned char *)dst;
    unsigned char *end = p + n;

    while (p != end)
        *q++ = *p++;
    return dst;
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
