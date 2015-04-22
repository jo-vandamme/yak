#include <yak/config.h>
#include <yak/lib/types.h>
#include <yak/video/terminal.h>
#include <yak/video/video.h>

#define LOG "\33\x0a\xf0term  >\33r"

static struct surface term_surf[NUM_TERM];
static struct renderer term_rend[NUM_TERM];

static int current_term = 0;

void term_init(int term, vbe_mode_info_t *mode_info, 
               int x, int y, int w, int h, int fg_color, int bg_color)
{
    term_surf[term].fb_w = mode_info->res_x;
    term_surf[term].fb_h = mode_info->res_y;
    term_surf[term].fb_bpp = mode_info->bpp;
    term_surf[term].ox = x;
    term_surf[term].oy = y;
    term_surf[term].x = term_surf[term].y = 0;
    term_surf[term].w = w;
    term_surf[term].h = h;
    term_surf[term].fg_color = fg_color;
    term_surf[term].bg_color = bg_color;

    term_surf[term].framebuffer = term_surf[term].backbuffer = (unsigned char *)LFB_BASE;

    term_surf[term].rend = &term_rend[term];
    surface_init(&term_surf[term]);

    if ((bg_color & 0x00ffffff) != 0)
        term_surf[term].rend->clear(&term_surf[term]);
}

int term_set_current(int term)
{
    int last_term = current_term;
    current_term = term;
    return last_term;
}

void term_putc(const char c)
{
    term_surf[current_term].rend->putc(&term_surf[current_term], c);
}

inline void term_puts(const char *str)
{
    int color = term_surf[current_term].fg_color;
    while (*str) {
        if (*str == '\33') {
            if (*++str == 'r') {
                term_surf[current_term].fg_color = color;
                str++;
            } else {
                uint8_t a = *str & 0xf0;
                uint8_t r = *str & 0x0f; ++str;
                uint8_t g = *str & 0xf0;
                uint8_t b = *str & 0x0f; ++str;
                term_surf[current_term].fg_color = a << 24 | r << 20 | g << 8 | b << 4;
            }
        } else {
            term_putc(*str++);
        }
    }
    term_surf[current_term].fg_color = color;
}

void term_clear(void)
{
    term_surf[current_term].rend->clear(&term_surf[current_term]);
}

int term_fg_color(int c)
{
    int old_color = term_surf[current_term].fg_color;
    term_surf[current_term].fg_color = c;
    return old_color;
}

int term_bg_color(int c)
{
    int old_color = term_surf[current_term].bg_color;
    term_surf[current_term].bg_color = c;
    return old_color;
}

struct surface *term_surface(void)
{
    return &term_surf[current_term];
}
