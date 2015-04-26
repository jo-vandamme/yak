#include <yak/kernel.h>
#include <yak/lib/rand.h>

// Marsaglia's xorshift96 generator
static unsigned long x = 123456789, y = 362436069, z = 521288629;

int rand(void)
{
    unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;
    return (int)z;
}
