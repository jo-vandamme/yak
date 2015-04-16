#include <yak/lib/string.h>
#include <yak/lib/types.h>

#include <yak/video/video.h>
#include <yak/video/font8x8.h>

#define LINE_SPACE 4
#define FONT_SPACE 0
#define LINES_TO_SCROLL 5

static void putch32(struct surface *surf, char c, int color)
{
    const unsigned char * const font = font8x16[(uint8_t)c];
    unsigned int *const video = (unsigned int *)surf->backbuffer;
    const int pitch = surf->fb_w;
    volatile unsigned where = (surf->ox + surf->x) + (surf->oy + surf->y) * pitch;
    
    for (unsigned int h = 0; h < FONT_HEIGHT; ++h) {
        for (unsigned int w = 0; w < FONT_WIDTH; ++w) {
            if (font[h] & 1 << w) {
                video[where] = color;
            }
            ++where;
        }
        where += pitch - FONT_WIDTH;
    }
    surf->x += FONT_WIDTH + FONT_SPACE;
}

static void putch(struct surface *surf, char c)
{
    if (!surf || c == '\0')  {
        return;
    }

    if (surf->x + FONT_WIDTH > surf->w || c == '\n') {
        surf->x = 0;
        surf->y += FONT_HEIGHT + LINE_SPACE;
    }
    if (surf->y + FONT_HEIGHT > surf->h) {
        surf->rend->scroll(surf);
        //surf->rend->clear(surf);
        surf->y -= (FONT_HEIGHT + LINE_SPACE) * LINES_TO_SCROLL;
    }

    if (c == '\t') {
        do {
            putch32(surf, ' ', surf->fg_color);
        } while ((surf->x % (4 * (FONT_WIDTH + FONT_SPACE))) != 0);
    }
    
    if (isprint(c))
        putch32(surf, c, surf->fg_color);
}


static void clear(struct surface *surf)
{
    uint32_t *ptr = (uint32_t *)surf->backbuffer + surf->ox + surf->oy * surf->fb_w;

    for (uint32_t h = 0; h < surf->h; ++h) {
        for (uint32_t w = 0; w < surf->w; ++w, ++ptr) {
            *ptr = surf->bg_color;
        }
        ptr += surf->fb_w - surf->w;
    }
    surf->y %= (surf->h - FONT_HEIGHT - LINE_SPACE);
}
    
static void scroll(struct surface *surf)
{
    uint32_t *ptr = (uint32_t *)surf->backbuffer + surf->ox + surf->oy * surf->fb_w;

    // move lines upward
    uint32_t h = 0, w;
    const uint32_t line_height = FONT_HEIGHT + LINE_SPACE;
    const uint32_t pitch = line_height * LINES_TO_SCROLL * surf->fb_w;

    for (; h < (uint32_t)surf->h - line_height * LINES_TO_SCROLL; ++h) {
        for (w = 0; w < surf->w; ++w, ++ptr) {
            *ptr = *(ptr + pitch);
        }
        ptr += surf->fb_w - surf->w;
    }

    // clear last line(s)
    for (; h < (uint32_t)surf->h; ++h) {
        for (w = 0; w < surf->w; ++w, ++ptr) {
            *ptr = surf->bg_color;
        }
        ptr += surf->fb_w - surf->w;
    }
}

void surface_init(struct surface *surf)
{
    //init_font();

    if (!surf)
        return;

    if (surf->rend) {
        surf->rend->putc = putch;
        surf->rend->clear = clear;
        surf->rend->scroll = scroll;
    }
}
