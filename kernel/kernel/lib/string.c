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
    char *retval = (char *)dst;
    if (n < 8) {
		register size_t dummy;
		asm volatile(
			"rep; movsb"
			: "=&D"(dst), "=&S"(src), "=&c"(dummy)
			: "0"(dst), "1"(src), "2"(n)
			: "memory");
        return retval;
    }
	int d0, d1, d2;
	asm volatile(
        "rep movsq\n"
        "testb $4, %b4\n"
        "je 1f\n"
        "rep movsl\n"
		"1: testb $2, %b4\n"
		"je 2f\n"
		"movsw\n"
		"2: testb $1, %b4\n"
		"je 3f\n"
		"movsb\n"
		"3:"
		: "=&c"(d0), "=&D"(d1), "=&S"(d2)
		: "0"(n / 8), "q"(n), "1"(dst), "2"(src)
		: "memory");
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
