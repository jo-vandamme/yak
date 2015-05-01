#include <yak/lib/types.h>
#include <yak/lib/string.h>
#include <yak/video/video.h>
#include <yak/video/default_font.h>

#define LINE_SPACE 4
#define FONT_SPACE 0

static void putch32(struct surface *surf, char c, int fg_color, int bg_color)
{
    const unsigned char * const font = default_font[(uint8_t)c];
    unsigned int *const video = (unsigned int *)surf->backbuffer;
    const int pitch = surf->fb_w;
    volatile unsigned where = (surf->ox + surf->x) + (surf->oy + surf->y) * pitch;
    
    int color;
    for (unsigned int h = 0; h < FONT_HEIGHT; ++h) {
        for (unsigned int w = 0; w < FONT_WIDTH; ++w) {
            color = font[h] & (1 << w) ? fg_color : bg_color;
            video[where] = color;
            ++where;
        }
        where += pitch - FONT_WIDTH;
    }
    surf->x += FONT_WIDTH + FONT_SPACE;
}

static void putch(struct surface *surf, char c)
{
    if (!surf || c == '\0') 
        return;

    if (c == '\b') {
        if (surf->x == 0) {
            surf->x = surf->w - FONT_WIDTH - FONT_SPACE; 
            surf->x -= surf->x % (FONT_WIDTH + FONT_SPACE);
            surf->y -= FONT_HEIGHT + LINE_SPACE;
        } else {
            surf->x -= FONT_WIDTH + FONT_SPACE;
        }
    }
    if (surf->x + FONT_WIDTH > surf->w || c == '\n') {
        surf->x = 0;
        surf->y += FONT_HEIGHT + LINE_SPACE;
    }
    if (surf->y + FONT_HEIGHT > surf->h) {
        if (surf->scroll) {
            surf->rend->scroll(surf);
            surf->y -= (FONT_HEIGHT + LINE_SPACE) * surf->scroll;
        } else {
            surf->rend->clear(surf);
            surf->y %= (surf->h - FONT_HEIGHT - LINE_SPACE);
        }
    }
    if (c == '\t') {
        do {
            putch32(surf, ' ', surf->fg_color, surf->bg_color);
        } while ((surf->x % (4 * (FONT_WIDTH + FONT_SPACE))) != 0);
    }
    
    if (isprint(c))
        putch32(surf, c, surf->fg_color, surf->bg_color);
}


static void clear(struct surface *surf)
{
    uint32_t *ptr = (uint32_t *)surf->backbuffer + surf->ox + surf->oy * surf->fb_w;

    for (uint32_t h = 0; h < surf->h; ++h) {
        for (uint32_t w = 0; w < surf->w; ++w, ++ptr)
            *ptr = surf->bg_color;
        ptr += surf->fb_w - surf->w;
    }
}
    
static void scroll(struct surface *surf)
{
    uint32_t *ptr = (uint32_t *)surf->backbuffer + surf->ox + surf->oy * surf->fb_w;

    const uint64_t line_height = FONT_HEIGHT + LINE_SPACE;
    const uint64_t pitch = line_height * surf->scroll * surf->fb_w;

    // move lines upward
    uint32_t h, w;
    for (h = 0; h < (uint32_t)surf->h - line_height * surf->scroll; ++h) {
        for (w = 0; w < surf->w; ++w, ++ptr)
            *ptr = *(ptr + pitch);
        ptr += surf->fb_w - surf->w;
    }

    // clear last line(s)
    for (; h < (uint32_t)surf->h; ++h) {
        for (w = 0; w < surf->w; ++w, ++ptr)
            *ptr = surf->bg_color;
        ptr += surf->fb_w - surf->w;
    }
}

void surface_init(struct surface *surf)
{
    if (!surf)
        return;

    if (surf->rend) {
        surf->rend->putc = putch;
        surf->rend->clear = clear;
        surf->rend->scroll = scroll;
    }
}
