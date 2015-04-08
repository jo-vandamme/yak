#include <yak/arch/ports.h>
#include <yak/arch/pic.h>

#define MASTER_CTRL 0x20
#define SLAVE_CTRL  0xa0
#define MASTER_DATA 0x21
#define SLAVE_DATA  0xa1

#define ICW1_ICW4   0x01
#define ICW1_INIT   0x10
#define ICW4_8086   0x01

void pic_disable(void)
{
    // ICW1: start the initialization sequence (in cascade mode)
    outb(MASTER_CTRL, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(SLAVE_CTRL, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // ICW2: master and slave PIC vector offsets
    outb(MASTER_DATA, 0xe0);
    io_wait();
    outb(SLAVE_DATA, 0xe8);
    io_wait();

    // ICW3: tell master PIC there is a slave PIC at IRQ2 (0000 0100)
    outb(MASTER_DATA, 4);
    io_wait();
    // ICW3: tell slave PIC its cascade identity (0000 0010)
    outb(SLAVE_DATA, 2);
    io_wait();

    // ICW4: controls how everything is to operate
    outb(MASTER_DATA, ICW4_8086);
    io_wait();
    outb(SLAVE_DATA, ICW4_8086);
    io_wait();

    // mask all interrupts
    outb(MASTER_DATA, 0xff);
    io_wait();
    outb(SLAVE_DATA, 0xff);
    io_wait();
}
