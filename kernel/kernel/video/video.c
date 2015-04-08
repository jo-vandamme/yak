#include <yak/lib/string.h>
#include <yak/lib/types.h>

#include <yak/video/video.h>
#include <yak/video/font8x8.h>

#define LINE_SPACE 4
#define FONT_SPACE 0
#define LINES_TO_SCROLL 10

inline static void putch24(struct surface *surf, char c, int color)
{
    const unsigned char * const font = font8x8[(uint8_t)c];
    unsigned char * const video = surf->backbuffer;
    unsigned int pitch = surf->fb_w * (surf->fb_bpp >> 3);

    volatile unsigned where = 
        (surf->fb_bpp >> 3) * (surf->ox + surf->x) 
        + (surf->oy + surf->y) * pitch;

    uint32_t col0 = (color << 24) | ((color >>  0) & 0x00ffffff); // BGR B
    uint32_t col1 = (color << 16) | ((color >>  8) & 0x0000ffff); // GR BG
    uint32_t col2 = (color <<  8) | ((color >> 16) & 0x000000ff); // R BGR

    uint32_t *dest32 = (uint32_t *)video;
    unsigned int *fmask;

#define OPACITY 0xffffffff
    for (unsigned int h = 0; h < FONT_HEIGHT; ++h) {
        
        fmask = font_mask[font[h]];

        dest32 = (uint32_t *)&video[where];
        dest32[0] = (OPACITY & (dest32[0] & ~fmask[0])) | (col0 & fmask[0]); // BGR B
        dest32[1] = (OPACITY & (dest32[1] & ~fmask[1])) | (col1 & fmask[1]); // GR BG
        dest32[2] = (OPACITY & (dest32[2] & ~fmask[2])) | (col2 & fmask[2]); // R BGR
       
        dest32[3] = (OPACITY & (dest32[3] & ~fmask[3])) | (col0 & fmask[3]); // BGR G
        dest32[4] = (OPACITY & (dest32[4] & ~fmask[4])) | (col1 & fmask[4]); // GR BG
        dest32[5] = (OPACITY & (dest32[5] & ~fmask[5])) | (col2 & fmask[5]); // R BGR
        
        where += pitch;
    }
    surf->x += FONT_WIDTH + FONT_SPACE;
}

inline static void putch32(struct surface *surf, char c, int color)
{
    const unsigned char * const font = font8x8[(uint8_t)c];
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

inline static void putch_(struct surface *surf, char c, int color)
{
    if (surf->fb_bpp == 32)
        putch32(surf, c, color);
    else
        putch24(surf, c, color);
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
            putch_(surf, ' ', surf->fg_color);
        } while ((surf->x % (4 * (FONT_WIDTH + FONT_SPACE))) != 0);
    }
    
    if (isprint(c))
        putch_(surf, c, surf->fg_color);
}


static void clear(struct surface *surf)
{
    unsigned char * const video = surf->backbuffer;
    unsigned int pitch = surf->fb_w * (surf->fb_bpp >> 3);
    volatile unsigned where = (surf->fb_bpp >> 3) * surf->ox + surf->oy * pitch;

    unsigned char b = surf->bg_color & 0xff;
    unsigned char g = (surf->bg_color >> 8) & 0xff;
    unsigned char r = (surf->bg_color >> 16) & 0xff;

    for (unsigned int h = 0; h < surf->h; ++h) {
        for (unsigned int w = 0; w < surf->w; ++w) {
            video[where + 0] = b;
            video[where + 1] = g;
            video[where + 2] = r;
            where += surf->fb_bpp >> 3;
        }
        where += pitch - surf->w * (surf->fb_bpp >> 3);
    }
    //surf->y %= (surf->h - FONT_HEIGHT - LINE_SPACE);
}
    
static void scroll(struct surface *surf)
{
    unsigned char * const video = surf->backbuffer;
    unsigned int pitch = surf->fb_w * (surf->fb_bpp >> 3);
    volatile unsigned where = (surf->fb_bpp >> 3) * surf->ox + surf->oy * pitch;

    /* move lines upward */
    unsigned int h, line_height = FONT_HEIGHT + LINE_SPACE;
    unsigned int pitch_ = line_height * LINES_TO_SCROLL * pitch;
    for (h = 0; h < (unsigned int)surf->h - line_height * LINES_TO_SCROLL; ++h) {
        for (unsigned int w = 0; w < surf->w; ++w) {
            video[where + 0] = video[where + pitch_ + 0];
            video[where + 1] = video[where + pitch_ + 1];
            video[where + 2] = video[where + pitch_ + 2];
            where += surf->fb_bpp >> 3;
        }
        where += pitch - surf->w * (surf->fb_bpp >> 3);
    }
    /* clear last line(s) */
    unsigned char b = surf->bg_color & 0xff;
    unsigned char g = (surf->bg_color >> 8) & 0xff;
    unsigned char r = (surf->bg_color >> 16) & 0xff;
    for (; h < (unsigned int)surf->h; ++h) {
        for (unsigned int w = 0; w < surf->w; ++w) {
            video[where + 0] = b;
            video[where + 1] = g;
            video[where + 2] = r;
            where += surf->fb_bpp >> 3;
        }
        where += pitch - surf->w * (surf->fb_bpp >> 3);
    }
}

void surface_init(struct surface *surf)
{
    init_font();

    if (!surf)
        return;

    if (surf->rend) {
        surf->rend->putc = putch;
        surf->rend->clear = clear;
        surf->rend->scroll = scroll;
    }
}
