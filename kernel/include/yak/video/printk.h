#ifndef __YAK_PRINTK_H__
#define __YAK_PRINTK_H__

#include <stdarg.h>

int printk(const char *fmt, ...);
void panic(const char *fmt, ...);

#endif
