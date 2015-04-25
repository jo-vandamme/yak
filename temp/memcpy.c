#include <yak/kernel.h>
#include <yak/lib/string.h>

// For small memory blocks (< 256 bytes) this version (rep stosb) is faster
#define small_memcpy(dest, src, count) \
	{ \
		register size_t dummy; \
		asm volatile( \
			"rep; movsb" \
			: "=&D"(dest), "=&S"(src), "=&c"(dummy) \
			: "0"(dest), "1"(src), "2"(count) \
			: "memory"); \
	}

void *memcpy_sse(void *restrict dst, const void *restrict src, size_t count) 
{
    void *retval = dst;
    char *d = (char *)dst;
    const char *s = (const char *)src;

    if (count >= 256) {

        asm volatile(
                "prefetchnta    (%0)\n"
                "prefetchnta  32(%0)\n"
                "prefetchnta  64(%0)\n"
                "prefetchnta  96(%0)\n"
                "prefetchnta 128(%0)\n"
                "prefetchnta 160(%0)\n"
                "prefetchnta 192(%0)\n"
                "prefetchnta 224(%0)\n"
                "prefetchnta 256(%0)\n"
                "prefetchnta 288(%0)\n"
                "prefetchnta 320(%0)\n"
                "prefetchnta 352(%0)\n"
                :: "r"(s));

#define SSE_REG_SIZE 128

        // copy bytes until dst is aligned to SSE_REG_SIZE
        register size_t delta = ((size_t)d) & (SSE_REG_SIZE - 1);
        if (delta) {
            delta = SSE_REG_SIZE - delta;
            count -= delta;
            small_memcpy(d, s, delta);
        }

        size_t nblocks = count >> 7; // divide by 128
        count &= 127; // remaining bytes

        if (((size_t)src) & 15) { // src isn't aligned on a 16-byte boundary
            for (; nblocks > 0; --nblocks, s += 128, d += 128) {
                asm volatile (
                        "prefetchnta 384(%0)\n"
                        "prefetchnta 416(%0)\n"
                        "prefetchnta 448(%0)\n"
                        "prefetchnta 480(%0)\n"
                        "movdqu    (%0),  %%xmm0\n"
                        "movdqu  16(%0),  %%xmm1\n"
                        "movdqu  32(%0),  %%xmm2\n"
                        "movdqu  48(%0),  %%xmm3\n"
                        "movdqu  64(%0),  %%xmm4\n"
                        "movdqu  80(%0),  %%xmm5\n"
                        "movdqu  96(%0),  %%xmm6\n"
                        "movdqu 112(%0),  %%xmm7\n"
                        "movntdq %%xmm0,    (%1)\n"
                        "movntdq %%xmm1,  16(%1)\n"
                        "movntdq %%xmm2,  32(%1)\n"
                        "movntdq %%xmm3,  48(%1)\n"
                        "movntdq %%xmm4,  64(%1)\n"
                        "movntdq %%xmm5,  80(%1)\n"
                        "movntdq %%xmm6,  96(%1)\n"
                        "movntdq %%xmm7, 112(%1)\n"
                        :: "r"(s), "r"(d) : "memory");
            }
        } else { // src is aligned on a 16-byte boundary
            for (; nblocks > 0; --nblocks, s += 128, d += 128) {
                asm volatile (
                        "prefetchnta 384(%0)\n"
                        "prefetchnta 416(%0)\n"
                        "prefetchnta 448(%0)\n"
                        "prefetchnta 480(%0)\n"
                        "movdqa    (%0),  %%xmm0\n"
                        "movdqa  16(%0),  %%xmm1\n"
                        "movdqa  32(%0),  %%xmm2\n"
                        "movdqa  48(%0),  %%xmm3\n"
                        "movdqa  64(%0),  %%xmm4\n"
                        "movdqa  80(%0),  %%xmm5\n"
                        "movdqa  96(%0),  %%xmm6\n"
                        "movdqa 112(%0),  %%xmm7\n"
                        "movntdq %%xmm0,    (%1)\n"
                        "movntdq %%xmm1,  16(%1)\n"
                        "movntdq %%xmm2,  32(%1)\n"
                        "movntdq %%xmm3,  48(%1)\n"
                        "movntdq %%xmm4,  64(%1)\n"
                        "movntdq %%xmm5,  80(%1)\n"
                        "movntdq %%xmm6,  96(%1)\n"
                        "movntdq %%xmm7, 112(%1)\n"
                        :: "r"(s), "r"(d) : "memory");
            }
        }
        // movntdq is weakly-ordered, a "sfence" is needed to become ordered again
        asm volatile ("sfence" ::: "memory");
        //asm volatile ("emms" ::: "memory"); // enables to use FPU
    }

    // copy the remaining bytes
	if (count) {

	    if (count < 8) {
		    small_memcpy(d, s, count);
		    return retval;
	    }

	    int d0, d1, d2;
	    asm volatile(
		    "rep movsq\n\t"
            "testb $4, %b4\n\t"
            "je 1f\n\t"
            "rep movsl\n"
		    "1: testb $2, %b4\n\t"
		    "je 2f\n\t"
		    "movsw\n"
		    "2: testb $1, %b4\n\t"
		    "je 3f\n\t"
		    "movsb\n"
		    "3:"
		    : "=&c"(d0), "=&D"(d1), "=&S"(d2)
		    : "0"(count / 8), "q"(count), "1"(d), "2"(s)
		    : "memory");
    }

    return retval;
}
