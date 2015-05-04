#include <yak/kernel.h>
#include <yak/initcall.h>
#include <yak/lib/string.h>
#include <yak/boot/multiboot.h>
#include <yak/video/vbe.h>
#include <yak/video/terminal.h>
#include <yak/cpu/idt.h>
#include <yak/cpu/mp.h>
#include <yak/cpu/interrupt.h>
#include <yak/mem/mem.h>
#include <yak/mem/pmm.h>
#include <yak/mem/vmm.h>
#include <yak/arch/acpi.h>
#include <yak/arch/tsc.h>
#include <yak/arch/pit.h>
#include <yak/arch/lapic.h>
#include <yak/dev/keyboard.h>

#define LOG LOG_PREFIX("main", 4)

multiboot_info_t *mbi;
vbe_mode_info_t *mode_info;

extern unsigned int KERN_BNUM;

INIT_CODE void init_system(u64_t magic, u64_t mboot)
{
    mbi = (multiboot_info_t *)((u64_t)mboot + VIRTUAL_BASE);
    mode_info = (vbe_mode_info_t *)(mbi->vbe_mode_info + VIRTUAL_BASE);

    int margin = 10;
    term_init(0, mode_info, margin, margin, mode_info->res_x - 2*margin, 
            mode_info->res_y - 2*margin, 0xffffff, 0x000000, 1);

    printk("\33\x03\xfaYAK Kernel\33r build %u compiled on " __DATE__ " " __TIME__ " using gcc-" __VERSION__ ".\n" \
           "\33\x03\xfa==========\33r Copyright 2015-2016: Jonathan Vandamme. All rights reserved.\n\n", &KERN_BNUM);

    if (magic != MBOOT_LOADER_MAGIC)
        panic("Bad multiboot magic value\n");

    printk(LOG "resolution %ux%ux%u\n", mode_info->res_x, mode_info->res_y, mode_info->bpp);
    
    // we should not allocate memory before mem_init(),
    // which means that some PML3...PML1 tables must be set statistically.
    // By default, we map more memory than needed, mem_init() will then do a cleanup
    isr_stubs_init();
    isr_handlers_init();
    idt_init();
    tsc_init();
    uintptr_t madt = acpi_init();
    mp_init0(madt);
    mem_init();
    mp_init1(); 
    kbd_init();
}

#include <yak/arch/spinlock.h>
static spinlock_t func_lock;

void func(__unused void *r)
{
    if (lapic_id() != 0)
        return;
    spin_lock(&func_lock);

    int x, y;
    term_get_xy(&x, &y);
    term_set_xy(mode_info->res_x - 200, 0);
    printk("%016x", read_tsc());
    term_set_xy(x, y);

    spin_unlock(&func_lock);
}

#include <yak/lib/pool.h>

struct mystruct {
    int a;
    int b;
    int c;
    int d;
};

POOL_DECLARE(mypool, struct mystruct, 100);

void pool_func(__unused void *r)
{
    struct mystruct *m0 = POOL_ALLOC(mypool);
    struct mystruct *m1 = POOL_ALLOC(mypool);
    struct mystruct *m2 = POOL_ALLOC(mypool);
    struct mystruct *m3 = POOL_ALLOC(mypool);

    printk("%p %p %p %p\n", m0, m1, m2, m3);

    POOL_FREE(mypool, m0);
    POOL_FREE(mypool, m1);

    m0 = POOL_ALLOC(mypool);
    m1 = POOL_ALLOC(mypool);
    struct mystruct *m4 = POOL_ALLOC(mypool);
    struct mystruct *m5 = POOL_ALLOC(mypool);

    printk("%p %p %p %p\n", m0, m1, m4, m5);
}

void kernel_main(u64_t magic, u64_t mboot)
{
    init_system(magic, mboot);

    mem_reclaim_init();
    print_mem_stat_global();

    // kernel initialized - load ramdisk
    // and start init process here

    //isr_register(0x20, func);

    printk("\n");
    POOL_INIT(mypool);
    isr_register(60, pool_func);
    lapic_send_ipi(0, 60);
    lapic_send_ipi(1, 60);
    lapic_send_ipi(2, 60);
    lapic_send_ipi(3, 60);
    lapic_send_ipi(4, 60);

    for (;;) {
        if (kbd_lastchar() == 'q')
            kbd_reset_system();
    }
}

