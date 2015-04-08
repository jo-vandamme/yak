#include <stdarg.h>

#include <yak/lib/format.h>
#include <yak/video/terminal.h>
#include <yak/video/printk.h>

int printk(const char *fmt, ...)
{
    char buf[1024];
    va_list args;
    int n, c;

    va_start(args, fmt);
    n = vsprintf(buf, fmt, args);
    va_end(args);

    c = term_fg_color(0x30f030);
    //term_puts("[kernel] ");
    term_fg_color(c);

    term_puts(buf);

    return n;
}

void panic(const char *fmt, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    term_fg_color(0xff0000);
    term_puts("\n[PANIC]\n");
    term_puts(buf);

    asm volatile("cli");
    for (;;) { };
}
