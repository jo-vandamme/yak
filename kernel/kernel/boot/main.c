#include <yak/kernel.h>
#include <yak/config.h>
#include <yak/initcall.h>
#include <yak/lib/string.h>
#include <yak/boot/multiboot.h>
#include <yak/video/vbe.h>
#include <yak/video/terminal.h>
#include <yak/cpu/idt.h>
#include <yak/cpu/mp.h>
#include <yak/mem/mem.h>
#include <yak/arch/acpi.h>
#include <yak/arch/tsc.h>
#include <yak/arch/pit.h>
#include <yak/arch/lapic.h>
#include <yak/dev/keyboard.h>

#define LOG "\33\x0a\360main  ::\33r"

multiboot_info_t *mbi;
vbe_mode_info_t *mode_info;

INIT_CODE void init_system(u64_t magic, u64_t mboot)
{
    mbi = (multiboot_info_t *)((u64_t)mboot + VIRTUAL_BASE);
    mode_info = (vbe_mode_info_t *)(mbi->vbe_mode_info + VIRTUAL_BASE);

    int padding = 10;
    int term_w = mode_info->res_x - padding * 2;
    int term_h = mode_info->res_y - padding * 2;
    term_init(0, mode_info, padding, padding, term_w, term_h, 0xc0c0c0, 0x000000);

    if (magic != MBOOT_LOADER_MAGIC)
        panic("Bad multiboot magic value\n");

    printk(LOG " resolution %ux%ux%u\n", mode_info->res_x, mode_info->res_y, mode_info->bpp);
    
    // we should not allocate memory before mem_init()
    // this means that some PML3...PML1 tables must be set statistically
    // by default, we map more memory than needed, mem_init() will 
    // then do a cleanup
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
    //if (lapic_id() == 0)
    //    printk(".");
}

#include <yak/cpu/interrupt.h>

void kernel_main(u64_t magic, u64_t mboot)
{
    init_system(magic, mboot);

    mem_reclaim_init();

    //isr_register(80, reclaim);
    //lapic_send_ipi(3, 80);

    isr_register(0x20, func);

    for (;;) {
        //if (kbd_lastchar() == 'q')
        //    kbd_reset_system();
    }
}

/*
// Marsaglia's xorshift generator
static unsigned long x = 123456789, y = 362436069, z = 521288629;
unsigned long xorshift96(void)
{
    unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;
    return z;
}
*/
