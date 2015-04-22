#ifndef __YAK_PRINTK_H__
#define __YAK_PRINTK_H__

#include <stdarg.h>

void printk(const char *fmt, ...);
void panic(const char *fmt, ...);

#endif
