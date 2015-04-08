#include <yak/arch/ports.h>

inline void outb(short port, char data)
{
    asm volatile("outb %1, %0" : : "dN"(port), "a"((unsigned char)data));
}

inline void outw(short port, short data)
{
    asm volatile("outw %1, %0" : : "dN"(port), "a"(data));
}

inline void outl(short port, int data)
{
    asm volatile("outl %1, %0" : : "dN"(port), "a"(data));
}

inline char inb(short port)
{
    char ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

inline short inw(short port)
{
    short ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

inline int inl(short port)
{
    int ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

inline void io_wait(void)
{
    asm volatile("jmp 1f\n"
                 "1: jmp 2f\n"
                 "2:");
}
