#ifndef __YAK_KEYBOARD_H__
#define __YAK_KEYBOARD_H__

#include <yak/lib/types.h>

void kbd_init(void);
uint8_t kbd_getchar(void);
uint8_t kbd_lastchar(void);

void kbd_set_leds(const bool scroll, const bool num, const bool caps);
void kbd_reset_system(void);

#endif
