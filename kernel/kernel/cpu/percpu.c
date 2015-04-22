#include <yak/lib/string.h>
#include <yak/initcall.h>
#include <yak/video/printk.h>
#include <yak/mem/vmm.h>
#include <yak/cpu/percpu.h>

#define LOG "\33\x0a\360cpu   ::\33r"

// each percpu area begins with this header
struct percpu_header
{
    uintptr_t self;
    unsigned int id;
};

// this must be placed at the top of the percpu section
struct percpu_header __attribute__((section(".data.percpu.header"))) __percpu_header;

extern const char kernel_percpu_start[];
extern const char kernel_percpu_end[];

// this is executed by each core
INIT_CODE void percpu_init(unsigned int id, uintptr_t percpu_base)
{
    size_t percpu_size = (uintptr_t)kernel_percpu_end - (uintptr_t)kernel_percpu_start;
    memset((void *)percpu_base, 0, percpu_size);

    set_gs(VMM_V2P((uint64_t)percpu_base));

    struct percpu_header *header = (struct percpu_header *)percpu_base;
    header->self = percpu_base;
    header->id = id;

    //printk(LOG " percpu area ready for core %u [%016x]\n", id, percpu_base);
}
