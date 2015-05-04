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

#define LOG LOG_PREFIX("main", 2)

multiboot_info_t *mbi;
vbe_mode_info_t *mode_info;

extern unsigned int KERN_BNUM;

INIT_CODE void init_system(u64_t magic, u64_t mboot)
{
    mbi = (multiboot_info_t *)((u64_t)mboot + VIRTUAL_BASE);
    mode_info = (vbe_mode_info_t *)(mbi->vbe_mode_info + VIRTUAL_BASE);

    int margin = 10;
    term_init(0, mode_info, margin, margin, mode_info->res_x - 2*margin, 
            mode_info->res_y - 2*margin, 0xc0c0c0, 0x000000, 1);

    printk("\33\x06\x6fYAK Kernel\33\x08\x88 build %u compiled on " 
            __DATE__ " " __TIME__ " using gcc-" __VERSION__ ".\n\n", &KERN_BNUM);

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

    // kernel initialized - load ramdisk
    // and start init process here

    isr_register(0x20, func);

    POOL_INIT(mypool);
    isr_register(60, pool_func);

    int i;
    const unsigned size = 100;
    char str[size];
    char *command;
    int bg = 0x000000, fg = 0xc0c0c0;

    printk("\n");
    for (;;) {
        printk("\33\x06\xf6yak $ ");

        for (i = 0; (str[i] = (char)kbd_getchar()) != '\n'; ++i) { }
        str[i] = '\0';
        for (--i; i != 0 && (str[i] == ' ' || str[i] == '\t'); --i)
            str[i] = '\0';
        for (command = str; *command == ' ' || *command == '\t'; ++command) { }

        if (strncmp(command, "exit", size) == 0) {
            printk("Rebooting!\n");
            pit_mdelay(200);
            kbd_reset_system();
        }
        else if (strncmp(command, "uname", size) == 0)
            printk("YAK v0.1\n");
        else if (strncmp(command, "testpool", size) == 0)
            lapic_send_ipi(0, 60);
        else if (strncmp(command, "clear", size) == 0)
            term_clear();
        else if (strncmp(command, "free", size) == 0)
            print_mem_stat_global();
        else if (strncmp(command, "invert", size) == 0) {
            bg = ~bg;
            fg = ~fg;
            term_bg_color(bg);
            term_fg_color(fg);
        }
        else if (*command)
            printk("\33\x08\x88Unknown command [%s]\n", command);
    }
}

