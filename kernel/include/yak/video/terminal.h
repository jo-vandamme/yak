#ifndef __YAK_TERMINAL_H__
#define __YAK_TERMINAL_H__

#include <yak/video/vbe.h>
#include <yak/video/video.h>

void term_init(int term, vbe_mode_info_t *mode_info, 
               int x, int y, int w, int h, int fg_color, int bg_color, int scroll);
int term_set_current(int term);
void term_putc(const char c);
void term_puts(const char *str);
void term_clear(void);
int term_fg_color(int color);
int term_bg_color(int color);
void term_set_xy(int x, int y);
void term_get_xy(int *x, int *y);
struct surface *term_surface(void);

#endif
