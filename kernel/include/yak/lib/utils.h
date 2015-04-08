#ifndef __YAK_UTILS_H__
#define __YAK_UTILS_H__

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define align_down(x, align) ((x) - ((x) % (align)))
#define align_up(x, align) (((x) + (align) - 1) & ~((align) - 1))

#define byte2mb(x) ((x) >> 20)
#define byte2kb(x) (((x) - ((x) >> 20 << 20)) >> 10)

#endif
