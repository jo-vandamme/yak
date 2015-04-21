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

    printk("\33\x0f\xf0YAK is booting \33\x0f\xff[%ux%ux%u]\n", mode_info->res_x, mode_info->res_y, mode_info->bpp);
    
    // we should not allocate memory before mem_init()
    // this means that some PML3...PML1 tables must be set statistically
    // by default, we map more memory than needed, mem_init() will 
    // then do a cleanup
    isr_init();
    idt_init();
    tsc_init();
    mp_init(acpi_init()); // this will call mem_init()
    kbd_init();
}

static unsigned char *num2str(unsigned char *buf, unsigned radix, unsigned long long num, int upper)
{
    unsigned long i;
    do {
        i = (unsigned long)num % radix;
        buf--;
        *buf = (i < 10) ? i + '0' : (upper ? i - 10 + 'A' : i - 10 + 'a');
        num = (unsigned long long)num / radix;
    } while (num != 0);
    return buf;
}

void func(void *r)
{
    (void)r;
    //if (lapic_id() == 0)
    //    printk(".");
}

#include <yak/cpu/interrupt.h>
#include <yak/lib/sort.h>

/* marsaglia's xorshift generator */
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

int cmp_long(void *a, void *b)
{
    return *(long *)a - *(long *)b;
}

void kernel_main(u64_t magic, u64_t mboot)
{
    init_system(magic, mboot);

    reclaim_init_mem();

    unsigned char buf[100] = { 0 };
    unsigned char *ptr = buf + 100;
    unsigned char *ptr2 = num2str(ptr, 2, 0xdeadbabacafecaca, 0);
    printk("%s\n", ptr2);

    long tab1[50];
    long tab2[50];
    for (unsigned i = 0; i < sizeof(tab1) / sizeof(*tab1); ++i) {
        tab1[i] = ((float)xorshift96() / (float)UINT64_MAX) * 100;
        tab2[i] = tab1[i];
        printk("\033\x0f\xf0%u ", tab1[i]);
    }
    printk("\n");
    
    qsort(tab2, sizeof(tab2) / sizeof(*tab2), sizeof(*tab2), cmp_long);

    for (unsigned i = 0; i < sizeof(tab2) / sizeof(*tab2); ++i)
        printk("%u ", tab2[i]);
    printk("\n");

    //isr_register(80, reclaim);
    //lapic_send_ipi(3, 80);

    isr_register(0x20, func);

    for (;;) {
        //if (kbd_lastchar() == 'q')
        //    kbd_reset_system();
    }
}
