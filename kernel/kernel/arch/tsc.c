#include <yak/lib/types.h>
#include <yak/initcall.h>
#include <yak/video/printk.h>
#include <yak/arch/pit.h>
#include <yak/arch/tsc.h>

#define logid "\33\x0a\xf0<tsc>\33r"

static uint64_t cpu_freq;

inline uint64_t read_tsc(void)
{
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

void tsc_udelay(uint64_t us)
{
    uint64_t delay_count = (cpu_freq * (us > 0 ? us : 1)) / 1000000;
    uint64_t start_count = read_tsc();
    //printk("startc %08x%08x delayc %08x%08x\n", start_count >> 32, start_count, delay_count >> 32, delay_count);

    uint64_t next_count, last_count, end_count;
    const uint64_t roll_over_count = UINT64_MAX;

    while (start_count >= delay_count) {
        // wait for roll-over
        next_count = start_count;
        do {
            last_count = next_count;
            // sched here
            next_count = read_tsc();
        } while (next_count < last_count);
        delay_count -= start_count;
        start_count = roll_over_count;
    }
    end_count = start_count - delay_count;
    if (end_count > 0) {
        // wait for end_count or roll-over
        next_count = start_count;
        do {
            last_count = next_count;
            next_count = read_tsc();
        } while ((next_count > end_count) && (next_count < last_count));
    }

    //while (read_tsc() < counter)
    //    asm volatile("pause" : : : "memory");
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
    uint64_t tsc_start, tsc_diff, diff_max = 0;//UINT64_MAX;

    for (int i = 0; i < 5; ++i) {
        tsc_start = read_tsc();
        pit_udelay(ms_delay * 1000);
        tsc_diff = tsc_start - read_tsc();
        diff_max = tsc_diff > diff_max ? tsc_diff : diff_max;
    }
    cpu_freq = diff_max * (1000 / ms_delay);

    printk("%s detected %u.%u MHz processor\n",
           logid, (uint32_t)cpu_freq / 1000000, (uint8_t)(cpu_freq % 1000000));
}
