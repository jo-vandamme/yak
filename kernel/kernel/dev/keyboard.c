#include <yak/kernel.h>
#include <yak/cpu/interrupt.h>
#include <yak/arch/ioapic.h>
#include <yak/arch/ports.h>
#include <yak/arch/pit.h>
#include <yak/dev/keymap.h>
#include <yak/dev/keyboard.h>

#define LOG LOG_COLOR0 "kbd:\33r"

#define IRQ_KBD 1

#define KBD_CTRL_STATUS 0x64
#define KBD_CTRL_CMD    0x64

#define KBD_ENC_INPUT   0x60
#define KBD_ENC_CMD     0x60

enum scancodes {
    CTRL_SC         = 0x1d,
    LSHIFT_SC       = 0x2a,
    RSHIFT_SC       = 0x36,
    ALT_SC          = 0x38,
    CAPSLOCK_SC     = 0x3a,
    META_SC         = 0x5b,
    NUMLOCK_SC      = 0x45
};

enum special_keys_flags {
    CTRL_KEY        = 1 << 0,
    ALT_KEY         = 1 << 1,
    SHIFT_KEY       = 1 << 2,
    CAPSLOCK_KEY    = 1 << 3,
    META_KEY        = 1 << 4,
    NUMLOCK_KEY     = 1 << 5,
};
static uint16_t special_keys = 0;

#define KBD_BUF_SIZE    32
static uint8_t last_char = 0;
static uint8_t kbd_buffer[KBD_BUF_SIZE];
static uint8_t read_idx = 0;
static volatile uint8_t write_idx = 0;

#define BUFFER_EMPTY    0
#define BUFFER_FULL     1
union status_reg {
    struct {
        uint8_t output_buffer   : 1; // 0 - empty, 1 - full
        uint8_t input_buffer    : 1; // 0 - empty, 1 - full
        uint8_t system_flag     : 1; // 0 - power on reset, 1 - successfull Basic Assurance Test
        uint8_t command_data    : 1; // 0 - last write was data, 1 - last write was a command   
        uint8_t kbd_locked      : 1; // 0 - locked, 1 - not locked
        uint8_t aux_buffer      : 1; // 0 - read from 0x60 is valid, 1 - mouse data
        uint8_t timeout         : 1; // 0 - ok flag, 1 - timeout
        uint8_t parity_error    : 1; // 0 - ok flag, 1 - parity error with last byte
    } __attribute__((packed));
    uint8_t raw;
};

union lights_state {
    struct {
        uint8_t scroll  : 1;
        uint8_t num     : 1;
        uint8_t caps    : 1;
    } __attribute__((packed));
    uint8_t raw;
};
static union lights_state lights;

// read the keyboard controller status
static inline union status_reg kbd_read_status(void)
{
    union status_reg status = { .raw = inb(KBD_CTRL_STATUS) };
    return status;
}

// read the keyboard encoder input buffer
static inline uint8_t kbd_read_buffer(void)
{
    return inb(KBD_ENC_INPUT);
}

#define KBD_ENC  0
#define KBD_CTRL 1

// send a command to the keyboard controller or to the keyboard encoder
static void kbd_send_command(const uint8_t command, const int flag)
{
    for (;;) {
        if (kbd_read_status().input_buffer == BUFFER_EMPTY)
            break;
    }
    if (flag == KBD_CTRL)
        outb(KBD_CTRL_CMD, command);
    else if (flag == KBD_ENC)
        outb(KBD_ENC_CMD, command);
}

void kbd_set_leds(const bool scroll, const bool num, const bool caps)
{
    uint8_t data = (caps << 2) | (num << 1) | scroll;
    kbd_send_command(0xed, KBD_ENC);
    kbd_send_command(data, KBD_ENC);
}

void kbd_reset_system(void)
{
    kbd_send_command(0xd1, KBD_CTRL);
    kbd_send_command(0xfe, KBD_ENC);
}

// TODO: use switch instead of if/else if/else...
static void kbd_handler(__unused void *r)
{
    uint8_t scancode = inb(KBD_ENC_INPUT);
        
    // if the top bit of the byte is set, a key has just been released
    if (scancode & 0x80) {
        scancode &= 0x7f;
        if (scancode == LSHIFT_SC || scancode == RSHIFT_SC) {
            special_keys &= ~SHIFT_KEY;
            if (!(special_keys & CAPSLOCK_KEY)) {
                lights.caps = 0;
                kbd_set_leds(lights.scroll, lights.num, lights.caps);
            }
        } else if (scancode == CTRL_SC) {
            special_keys &= ~CTRL_KEY;
        }

    } else { // a key has just been pressed
        if (scancode == LSHIFT_SC || scancode == RSHIFT_SC) {
            special_keys |= SHIFT_KEY;
            lights.caps = 1;
            kbd_set_leds(lights.scroll, lights.num, lights.caps);
        } else if (scancode == CAPSLOCK_SC) {
            special_keys ^= CAPSLOCK_KEY;
            lights.caps = !lights.caps;
            kbd_set_leds(lights.scroll, lights.num, lights.caps);
        } else if (scancode == NUMLOCK_SC) {
            special_keys ^= NUMLOCK_KEY;
            lights.num = !lights.num;
            kbd_set_leds(lights.scroll, lights.num, lights.caps);
        } else if (scancode == CTRL_SC) {
            special_keys |= CTRL_KEY;
        } else {
            if (scancode == 0x2e && (special_keys & CTRL_KEY)) {
                // trigger a breakpoint exception
                asm volatile ("int $3");
                return;
            }
            int maj = (special_keys & (SHIFT_KEY | CAPSLOCK_KEY)) != 0;
            //printk("[%08b %x %u]\n", special_keys, scancode, maj);
            printk("%c", keymap_us[maj][scancode]);

            last_char = keymap_us[maj][scancode];
            if (((write_idx + 1) % KBD_BUF_SIZE) != read_idx) {
                kbd_buffer[write_idx] = last_char;
                write_idx = (write_idx + 1) % KBD_BUF_SIZE;
            }
        }
    }
}

uint8_t kbd_getchar()
{
    uint8_t c;

    while (read_idx == write_idx) ;

    c = kbd_buffer[read_idx];
    read_idx = (read_idx + 1) % KBD_BUF_SIZE;
    return c;
}

uint8_t kbd_lastchar()
{
    uint8_t tmp = last_char;
    last_char = '\0';
    return tmp;
}

void kbd_init()
{
    (void)kbd_read_buffer;
    //printk("keyboard status = %x\n", kbd_read_status().raw);
    kbd_send_command(0xae, KBD_CTRL);
    
    isr_register(IRQ(IRQ_KBD), kbd_handler);
    ioapic_set_irq(IRQ_KBD, IRQ(IRQ_KBD), 0);
    printk(LOG " Keyboard initialized\n");

    kbd_set_leds(0, 0, 0);
}

