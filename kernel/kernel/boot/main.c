#include <yak/kernel.h>
#include <yak/initcall.h>
#include <yak/lib/string.h>
#include <yak/boot/multiboot.h>
#include <yak/video/vbe.h>
#include <yak/video/terminal.h>
#include <yak/cpu/idt.h>
#include <yak/cpu/mp.h>
#include <yak/mem/mem.h>
#include <yak/mem/pmm.h>
#include <yak/arch/acpi.h>
#include <yak/arch/tsc.h>
#include <yak/arch/pit.h>
#include <yak/arch/lapic.h>
#include <yak/dev/keyboard.h>

#define LOG LOG_COLOR0 "main:\33r"

multiboot_info_t *mbi;
vbe_mode_info_t *mode_info;

static unsigned int width;
static unsigned int height;

INIT_CODE void init_system(u64_t magic, u64_t mboot)
{
    mbi = (multiboot_info_t *)((u64_t)mboot + VIRTUAL_BASE);
    mode_info = (vbe_mode_info_t *)(mbi->vbe_mode_info + VIRTUAL_BASE);

    width = mode_info->res_x;
    height = mode_info->res_y;

    int padding = 5;
    int term_w = mode_info->res_x - padding * 2;
    int term_h = mode_info->res_y - padding * 2;
    term_init(0, mode_info, padding, padding, term_w, term_h, 0xc0c0c0, 0x000000, 1);

    if (magic != MBOOT_LOADER_MAGIC)
        panic("Bad multiboot magic value\n");

    printk("\33\x0f\xf0Yak kernel built on " __DATE__ " " __TIME__ " with gcc-" __VERSION__ "\33r\n");
    printk(LOG " resolution %ux%ux%u\n", mode_info->res_x, mode_info->res_y, mode_info->bpp);
    
    // we should not allocate memory before mem_init(),
    // which means that some PML3...PML1 tables must be set statistically.
    // By default, we map more memory than needed, mem_init() will then do a cleanup
    isr_init();
    idt_init();
    tsc_init();
    uintptr_t madt = acpi_init();
    mp_init0(madt);
    mem_init();
    mp_init1(); 
    kbd_init();
}

void func(__unused void *r)
{
    if (lapic_id() != 0)
        return;
    int x, y;
    term_get_xy(&x, &y);
    term_set_xy(0, 600);
    printk("%016x", read_tsc());
    term_set_xy(x, y);
}

#include <yak/cpu/interrupt.h>
#include <yak/lib/pool.h>

struct mystruct {
    int a;
    int b;
    int c;
    int d;
};

POOL_DECLARE(mypool, struct mystruct, 1000);

void pool_func(__unused void *r)
{
    struct mystruct *m0 = POOL_ALLOC(mypool);
    struct mystruct *m1 = POOL_ALLOC(mypool);

    printk("%p %p\n", m0, m1);

    POOL_FREE(mypool, m0);
    POOL_FREE(mypool, m1);
}

void kernel_main(u64_t magic, u64_t mboot)
{
    init_system(magic, mboot);

    mem_reclaim_init();

    POOL_INIT(mypool);

    //isr_register(80, reclaim);
    //lapic_send_ipi(3, 80);

    isr_register(0x20, func);
    isr_register(0x20, pool_func);
    print_mem_stat_global();

    for (;;) {
        //if (kbd_lastchar() == 'q')
        //    kbd_reset_system();
    }
}

