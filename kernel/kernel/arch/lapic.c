#include <yak/kernel.h>
#include <yak/initcall.h>
#include <yak/mem/pmm.h>
#include <yak/mem/vmm.h>
#include <yak/arch/pit.h>
#include <yak/arch/tsc.h>
#include <yak/arch/lapic.h>

#define LOG LOG_COLOR0 "lapic:\33r"

// Local APIC register offsets, divided by 4 for use as uint[] indices

#define LAPIC_ID        (0x0020 / 4)    // ID
#define LAPIC_VER       (0x0030 / 4)    // version
#define LAPIC_TPR       (0x0080 / 4)    // task priority register
#define LAPIC_ABR       (0x0090 / 4)    // arbitration priority register
#define LAPIC_PPR       (0x00a0 / 4)    // processor priority register
#define LAPIC_EOI       (0x00b0 / 4)    // EOI register
#define LAPIC_LDR       (0x00d0 / 4)    // logical destimation register
#define LAPIC_DFR       (0x00e0 / 4)    // destination format register
#define LAPIC_SVR       (0x00f0 / 4)    // spurious-interrupt vector register
#define LAPIC_ISR       (0x0100 / 4)    // ISR 0-255
#define LAPIC_TMR       (0x0180 / 4)    // TMR 0-255
#define LAPIC_IRR       (0x0200 / 4)    // IRR 0-255
#define LAPIC_ESR       (0x0280 / 4)    // error status register
#define LAPIC_ICR0      (0x0300 / 4)    // interrupt command register 0-31
#define LAPIC_ICR1      (0x0310 / 4)    // interrupt command register 32-63
#define LAPIC_LVT       (0x0320 / 4)    // local vector table
#define LAPIC_LVTT      (0x0320 / 4)    // timer (first entry of LVT)
#define LAPIC_LVTPC     (0x0340 / 4)    // performance counter LVT register
#define LAPIC_LINT0     (0x0350 / 4)    // local vector table (LINT0)
#define LAPIC_LINT1     (0x0360 / 4)    // local vector table (LINT1)
#define LAPIC_LVTE      (0x0370 / 4)    // error LVT register
#define LAPIC_TICR      (0x0380 / 4)    // timer initial count register
#define LAPIC_TCCR      (0x0390 / 4)    // timer current count register
#define LAPIC_TDCR      (0x03e0 / 4)    // timer divide configuration register

// -----------------------------------
// Interrupt command register
// -----------------------------------

// delivery mode (bits 8-10)
#define ICR_FIXED           0x00000000
#define ICR_LOWEST          0x00000100
#define ICR_SMI             0x00000200
#define ICR_NMI             0x00000400
#define ICR_INIT            0x00000500
#define ICR_STARTUP         0x00000600

// destination mode (bit 11)
#define ICR_PHYSICAL        0x00000000
#define ICR_LOGICAL         0x00000800

// delivery status (bit 12)
#define ICR_IDLE            0x00000000
#define ICR_SEND_PENDING    0x00001000

// level (bit 14)
#define ICR_DEASSERT        0x00000000
#define ICR_ASSERT          0x00004000

// trigger mode (bit 15)
#define ICR_EDGE            0x00000000
#define ICR_LEVEL           0x00008000

// destination shorthand (bits 18-19)
#define ICR_DEST_FIELD      0x00000000
#define ICR_SELF            0x00040000
#define ICR_ALL_INCL_SELF   0x00080000
#define ICR_ALL_EXCL_SELF   0x000c0000

// ------------------------------------
// Local vector table
// -----------------------------------

// Delivery mode
#define LVT_FIXED           0x00000000
#define LVT_NMI             0x00000400
#define LVT_EXTINT          0x00000700

// delivery status
#define LVT_IDLE            0x00000000
#define LVT_SEND_PENDING    0x00001000

// trigger mode
#define LVT_EDGE            0x00000000
#define LVT_LEVEL           0x00008000

// mask
#define LVT_NOT_MASKED      0x00000000
#define LVT_MASKED          0x00010000

// timer mode
#define LVT_ONE_SHOT        0x00000000
#define LVT_PERIODIC        0x00020000

// -----------------------------------

#define IRQ_SPURIOUS        0xff
#define IRQ_TIMER           0x20

char lapic_mapping[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

volatile uint32_t *lapic;
static unsigned long timer_freq;

static inline void lapic_write(const uint32_t index, const uint32_t value)
{
    lapic[index] = value;
    lapic[LAPIC_ID]; // wait for write to finish by reading
}

static inline uint32_t lapic_read(const uint32_t index)
{
    return lapic[index];
}

static INIT_CODE unsigned long lapic_timer_calib(void)
{
    // setup timer to one shot mode
    lapic_write(LAPIC_LVTT, LVT_MASKED | LVT_ONE_SHOT | IRQ_TIMER);
    lapic_write(LAPIC_TDCR, 0xb); // divide by 1

    unsigned int ticks, ticks_min = UINT32_MAX;
    unsigned int ms_delay = 10;
    for (int i = 0; i < 10; ++i) {
        lapic_write(LAPIC_TICR, UINT32_MAX);
        pit_udelay(ms_delay * 1000);
        ticks = UINT32_MAX - lapic_read(LAPIC_TCCR);
        ticks_min = ticks < ticks_min ? ticks : ticks_min;
    }
    return ticks_min * (1000u / ms_delay); // frequency = ticks per second
}

static INIT_CODE void lapic_set_timer(const unsigned int count)
{
    // setup the timer -  it repeatedly counts down at bus frequency
    // and then issues an interrupt
    lapic_write(LAPIC_LVTT, LVT_PERIODIC | IRQ_TIMER);
    lapic_write(LAPIC_TDCR, 0xb); // divide by 1
    lapic_write(LAPIC_TICR, count);
}

INIT_CODE void lapic_enable(const int bsp)
{
    // acknowledge any pending interrupt
    for (unsigned int i = 0; i <= 0xff / 32; ++i) {
        uint32_t value = lapic_read(LAPIC_ISR + i);
        for (unsigned int j = 0; j < 32; ++j) {
            if (value & (1 << j))
                lapic_ack_irq();
        }
    }

    // disable the timer during the setup
    //uint32_t timer = lapic_read(LAPIC_LVTT);
    //lapic_write(LAPIC_LVTT, LVT_MASKED);
    
    // set logical destination mode as flat mode, use 0x1 for each cpu
    lapic_write(LAPIC_DFR, 0xffffffff);
    lapic_write(LAPIC_LDR, (lapic_read(LAPIC_LDR) & 0x00ffffff) | 1 << 24);

    // disable logical interrupt lines
    //lapic_write(LAPIC_LINT1, LVT_NMI | (bsp ? 0 : LVT_MASKED));
    //lapic_write(LAPIC_LINT0, LVT_LEVEL | LVT_EXTINT | (bsp ? 0 : LVT_MASKED));

    // disable errors and performance counter overflow interrupts
    //lapic_write(LAPIC_LVTE, 0xff | LVT_MASKED);
    //lapic_write(LAPIC_LVTPC, 0xff | LVT_MASKED);

    // accept all priority levels, this enables all interrupts */
    lapic_write(LAPIC_TPR, 0);

    // enable local APIC and set spurious interrupt vector
    lapic_write(LAPIC_SVR, 0x100 | IRQ_SPURIOUS);

    // restore timer
    //lapic_write(LAPIC_LVTT, timer);
    lapic_set_timer(timer_freq / TIMER_FREQ);
    (void)bsp;
}

INIT_CODE void lapic_init(const uintptr_t lapic_base)
{
    free_frame(VMM_V2P(lapic_mapping));
    map((uintptr_t)lapic_mapping, lapic_base, 3);
    lapic = (uint32_t *)lapic_mapping;

    uint32_t id = (uint32_t)lapic_id();
    uint32_t v = lapic_read(LAPIC_VER);
    uint32_t ver = v & 0x000000ff;
    uint32_t max_lvt = (v & 0x00ff0000) >> 16;

    printk(LOG " base: %#016x version: %u, %u LVTs, LAPIC ID: %u\n", 
           lapic_base, ver, max_lvt, id);

    timer_freq = lapic_timer_calib();
    printk(LOG " detected %u MHz bus clock\n", timer_freq / 1000000);

    lapic_enable(1);
    //printk(LOG " local apic initialized\n");
}

uint8_t lapic_id(void)
{
    return (lapic_read(LAPIC_ID) & 0x0f000000) >> 24;
}

uint8_t lapic_version(void)
{
    return lapic_read(LAPIC_VER) & 0x000000ff;
}

static inline void lapic_wait_icr_idle(void)
{
    while (lapic_read(LAPIC_ICR0) & ICR_SEND_PENDING)
        asm volatile("pause" ::: "memory");
}

void lapic_send_init_ipi(const uint8_t lapic_id)
{
    lapic_write(LAPIC_ICR1, lapic_id << 24);
    lapic_write(LAPIC_ICR0,
            ICR_DEST_FIELD | ICR_LEVEL | ICR_ASSERT | ICR_PHYSICAL | ICR_INIT);
    lapic_wait_icr_idle();
}

void lapic_send_startup_ipi(const uint8_t lapic_id, const uintptr_t addr)
{
    const uint8_t vector = (addr >> 12) & 0xff;
    lapic_write(LAPIC_ICR1, lapic_id << 24);
    lapic_write(LAPIC_ICR0, 
            ICR_DEST_FIELD | ICR_LEVEL | ICR_ASSERT | ICR_PHYSICAL | ICR_STARTUP | vector);
    lapic_wait_icr_idle();
}

void lapic_send_ipi(const uint8_t lapic_id, const uint8_t vector)
{
    lapic_write(LAPIC_ICR1, lapic_id << 24);
    lapic_write(LAPIC_ICR0, 
            ICR_DEST_FIELD | ICR_ASSERT | ICR_PHYSICAL | ICR_FIXED | vector);
    lapic_wait_icr_idle();
}

inline void lapic_clear_error(void)
{
    lapic_write(LAPIC_ESR, 0);
}

inline void lapic_ack_irq(void)
{
    if (lapic) {
        // use 0 for future compatibility
        lapic_write(LAPIC_EOI, 0);
    }
}
