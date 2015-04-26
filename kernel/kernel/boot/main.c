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

INIT_CODE void init_system(u64_t magic, u64_t mboot)
{
    mbi = (multiboot_info_t *)((u64_t)mboot + VIRTUAL_BASE);
    mode_info = (vbe_mode_info_t *)(mbi->vbe_mode_info + VIRTUAL_BASE);

    int padding = 5;
    int term_w = mode_info->res_x - padding * 2;
    int term_h = mode_info->res_y - padding * 2;
    term_init(0, mode_info, padding, padding, term_w, term_h, 0xc0c0c0, 0x000000, 1);

    if (magic != MBOOT_LOADER_MAGIC)
        panic("Bad multiboot magic value\n");

    printk("\33\x0f\xf0Yak kernel built on " __DATE__ " " __TIME__ " with gcc-" __VERSION__ "\33r\n");
    //printk(LOG " resolution %ux%ux%u\n", mode_info->res_x, mode_info->res_y, mode_info->bpp);
    
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
#include <yak/lib/string.h>
#include <yak/lib/rand.h>

void *memcpy_0(void *restrict dst, const void *restrict src, size_t n)
{
    char *d = (char *)dst;
    const char *s = (const char *)src;
    while (n--)
        *d++ = *s++;
    return dst;
}

char src[1024 * 100];
char dst[1024 * 100];

void kernel_main(u64_t magic, u64_t mboot)
{
    init_system(magic, mboot);

    mem_reclaim_init();

    //isr_register(80, reclaim);
    //lapic_send_ipi(3, 80);

    isr_register(0x20, func);

    print_mem_stat_global();

    unsigned count = sizeof(src);
    unsigned loops = 1000;
    unsigned chunk = count / loops;
    printk("bytes to copy = %u, number of loops = %u, bytes per copy = %u\n", count, loops, chunk);

    for (unsigned i = 0; i < count / sizeof(long); ++i) {
        *(((unsigned long *)src) + i) = rand();
        *(((unsigned long *)dst) + i) = rand();
    }

    void *(*tab[])(void *, const void *, size_t) = { memcpy_0, memcpy };

    for (int k = 0; k < 6; ++k) {
        uint64_t start = read_tsc();
        for (unsigned i = 0; i < loops; ++i)
            (*tab[k % 2])(dst + i * chunk, src + i * chunk, chunk);
        uint64_t delta = read_tsc() - start;

        printk("elapsed cycles = %u\n", delta);// / tsc_cpu_freq());

        //if (memcmp(src, dst, count))
        //    printk("fucked up!\n");
    }

    for (;;) {
        //if (kbd_lastchar() == 'q')
        //    kbd_reset_system();
    }
}

