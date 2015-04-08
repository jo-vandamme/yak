#ifndef __FORMAT_H__
#define __FORMAT_H__

#include <stdarg.h>

int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list arg);

#endif
