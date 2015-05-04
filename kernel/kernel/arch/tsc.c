#include <yak/kernel.h>
#include <yak/initcall.h>
#include <yak/arch/pit.h>
#include <yak/arch/tsc.h>

#define LOG LOG_PREFIX("tsc", 3)

static uint64_t cpu_freq;

inline uint64_t read_tsc(void)
{
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

void tsc_udelay(uint64_t us)
{
    uint64_t delay = (cpu_freq * (us > 0 ? us : 1)) / 1000000ull;
    uint64_t stop = read_tsc() + delay;

    // the tsc counter wraps every 200 years on a 3GH processor
    // so we do not check for wrap around

    while (read_tsc() < stop)
        cpu_relax();
}

inline void tsc_mdelay(uint64_t ms)
{
    return tsc_udelay(ms * 1000);
}

inline uint64_t tsc_cpu_freq(void)
{
    return cpu_freq;
}

INIT_CODE void tsc_init(void)
{
    unsigned int ms_delay = 10;
    uint64_t tsc_start, tsc_diff, diff_max = 0;

    for (int i = 0; i < 5; ++i) {
        tsc_start = read_tsc();
        pit_udelay(ms_delay * 1000);
        tsc_diff = read_tsc() - tsc_start;
        diff_max = tsc_diff > diff_max ? tsc_diff : diff_max;
    }
    cpu_freq = diff_max * (1000u / ms_delay);

    printk(LOG "detected %u MHz processor\n", cpu_freq / 1000000);
}
