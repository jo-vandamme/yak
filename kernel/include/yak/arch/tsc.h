#ifndef __YAK_TSC_H__
#define __YAK_TSC_H__

#include <yak/lib/types.h>

uint64_t read_tsc(void);
void tsc_udelay(uint64_t us);
void tsc_mdelay(uint64_t ms);
uint64_t tsc_cpu_freq(void);
void tsc_init(void);

#endif
