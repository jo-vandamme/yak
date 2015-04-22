#include <stdarg.h>
#include <yak/lib/string.h>
#include <yak/lib/format.h>
#include <yak/lib/types.h>

#define PAD_ZEROES     (1 << 0)
#define NEGATIVE       (1 << 1)
#define PLUS           (1 << 2)
#define SPACE          (1 << 3)
#define LEFT_JUSTIFIED (1 << 4)
#define SPECIAL        (1 << 5)
#define LOWER_CASE     (1 << 6)

#define INT            (1 << 0)
#define SHORT          (1 << 1)
#define LONG           (1 << 2)
#define LONGLONG       (1 << 3)

// Converts a binary number to its string representation in any base from base 2 to base 36.
// Return a pointer in str that points past where the number was converted and appended
static char *num2str(char *str, unsigned long long num, unsigned radix, int width, int precision, int type)
{
    char sign;
    char tmp[64];

    if (radix < 2 || radix > 36)
        return 0;

    if (type & LEFT_JUSTIFIED)
        type &= ~PAD_ZEROES;

    char c = (type & PAD_ZEROES) ? '0' : ' ';

    if (type & NEGATIVE) {
        sign = '-';
    } else {
        sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
    }
    if (sign)
        --width;

    if (type & SPECIAL)
        width = (radix == 16 || radix == 2) ? (width - 2) : ((radix == 8) ? (width - 1) : width);

    int i = 0;
    if (num == 0)
        tmp[i++] = '0';
    else
        while (num != 0) {
            unsigned j = (unsigned)num % radix;
            tmp[i++] = (j < 10) ? j + '0' : (LOWER_CASE ? j - 10 + 'a' : j - 10 + 'A');
            num /= radix;
        }

    precision = (i > precision) ? i : precision;
    width -= precision;

    if (!(type & (PAD_ZEROES + LEFT_JUSTIFIED)))
        while (width-- > 0)
            *str++ = ' ';
    if (sign)
        *str++ = sign;
    if (type & SPECIAL) {
        if (radix == 2 || radix == 8 || radix == 16)
            *str++ = '0';
        if (radix == 2)
            *str++ = LOWER_CASE ? 'b' : 'B';
        if (radix == 16)
            *str++ = LOWER_CASE ? 'x' : 'X';
    }
    if (!(type & LEFT_JUSTIFIED))
        while (width-- > 0)
            *str++ = c;

    while (i < precision--)
        *str++ = '0';
    while (i-- > 0)
        *str++ = tmp[i];
    while (width-- > 0)
        *str++ = ' ';

    return str;
}

// Does the same as glibc atoi function except it modifies the string
// pointer to point past the number in the string
// s a pointer to a string, modified version points past the parsed
// numbers in the string
// returns the converted number value inside s
static int skip_atoi(const char **s)
{
    int i = 0;
    while (isdigit(**s))
        i = i * 10 + *((*s)++) - '0';
    return i;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
    char *str;
    for (str = buf; *fmt; fmt++)
    {
        if (*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }

        int flags = 0;
get_flags:
        fmt++;
        switch (*fmt)
        {
            case '-': flags |= LEFT_JUSTIFIED; goto get_flags;
            case '+': flags |= PLUS; goto get_flags;
            case ' ': flags |= SPACE; goto get_flags;
            case '0': flags |= PAD_ZEROES; goto get_flags;
            case '#': flags |= SPECIAL; goto get_flags;
        }

        int field_width = -1;
        if (isdigit(*fmt)) {
            field_width = skip_atoi(&fmt);
        } else if (*fmt == '*') {
            field_width = va_arg(args, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT_JUSTIFIED;
            }
        }

        int precision = -1;
        if (*fmt == '.') {
            fmt++;
            if (isdigit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*')
                precision = va_arg(args, int);
            precision = (precision < 0) ? 0 : precision;
        }

        switch (*fmt)
        {
            case 'c':
                if (!(flags & LEFT_JUSTIFIED))
                    while (--field_width > 0)
                        *str++ = ' ';
                *str++ = (unsigned char)va_arg(args, int);
                while (--field_width > 0)
                    *str++ = ' ';
                break;

            case 's': ;
                char *s = va_arg(args, char *);
                if (!s)
                    s = "NULL";

                int len = strlen(s);
                if (precision < 0)
                    precision = len;
                else if (len > precision)
                    len = precision;

                if (!(flags & LEFT_JUSTIFIED))
                    while (len < field_width--)
                        *str++ = ' ';
                for (int i = 0; i < len; i++)
                    *str++ = *s++;
                while (len < field_width--)
                    *str++ = ' ';
                break;

            case 'b':
                str = num2str(str, va_arg(args, unsigned long long), 2, field_width, precision, flags);
                break;

            case 'o':
                str = num2str(str, va_arg(args, unsigned long long), 8, field_width, precision, flags);
                break;

            case 'x':
                flags |= LOWER_CASE;
            case 'X': ;
                str = num2str(str, va_arg(args, unsigned long long), 16, field_width, precision, flags);
                break;
            
            case 'u':
                str = num2str(str, va_arg(args, unsigned long long), 10, field_width, precision, flags);
                break;
            
            case 'd':
            case 'i': ;
                signed long long num = va_arg(args, signed long long);
                if (num < 0) {
                    flags |= NEGATIVE;
                    num = -num;
                }
                str = num2str(str, va_arg(args, signed long long), 10, field_width, precision, flags);
                break;

            case 'p':
                if (field_width == -1) {
                    field_width = 16;
                    flags |= PAD_ZEROES;
                }
                str = num2str(str, (unsigned long long)va_arg(args, void *), 16, field_width, precision, flags);
                break;

            case 'n': ;
                int *ip = va_arg(args, int *);
                *ip = (str - buf);
                break;

            default:
                if (*fmt != '%')
                    *str++ = '%';
                if (*fmt)
                    *str++ = *fmt;
                else
                    --fmt;
                break;
        }
    }

    *str = '\0';

    return str - buf;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int n = vsprintf(buf, fmt, args);
    va_end(args);

    return n;
}
