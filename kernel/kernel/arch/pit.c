#include <yak/lib/types.h>
#include <yak/arch/ports.h>
#include <yak/arch/pit.h>

#define PIT_FREQ 0x1234ddul

enum {
    PIT_GATE2   = 0x1,  // bit 0 - PIT's GATE-2 input
    PIT_SPEAKER = 0x2,  // bit 1 - enable/disable speaker
    PIT_OUT2    = 0x20  // bit 5 - PIT's OUT-2
};

enum {
    PIT_COUNTER0 = 0x40,
    PIT_COUNTER1 = 0x41,
    PIT_COUNTER2 = 0x42,
    PIT_CONTROL  = 0x43
};

union pit_cmd {
    uint8_t raw;
    struct {
        uint8_t bcd   : 1;
        uint8_t mode  : 3;
        uint8_t rw    : 2;
        uint8_t timer : 2;
    } __attribute__((packed));
};

// the maximum delay is approximately 54ms (0xffff * 1000 / PIT_FREQ)
// due to the counter being limited to 16 bits
void pit_udelay(unsigned int us)
{
    // stop timer 2 by clearing the gate-2 pin
    uint8_t val = inb(0x61) & ~PIT_GATE2;
    outb(0x61, val);

    union pit_cmd cmd = { .raw = 0 };
    cmd.bcd = 0;   // don't use bcd format
    cmd.mode = 0;  // mode 0 - interrupt on terminal count
    cmd.rw = 3;    // will will write lsb then msb
    cmd.timer = 2; // use timer 2
    outb(PIT_CONTROL, cmd.raw);

    // set the counter divisor
    us = us > 0 ? us : 1; 
    unsigned int counter = (PIT_FREQ * us) / 1000000;
    counter = counter > UINT16_MAX ? UINT16_MAX  : counter;
    outb(PIT_COUNTER2, counter & 0xff);
    outb(PIT_COUNTER2, counter >> 8);

    // start timer 2 by raising the gate-2 pin, unlink out-2 from speaker
    val = (inb(0x61) | PIT_GATE2) & ~PIT_SPEAKER;
    outb(0x61, val);

    while ((inb(0x61) & PIT_OUT2) == 0)
        asm volatile("pause" ::: "memory");
}

void pit_mdelay(unsigned int ms)
{
    for (unsigned int i = 0; i < ms; ++i)
        pit_udelay(1000);
}
