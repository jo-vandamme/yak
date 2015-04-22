#ifndef __YAK_VIDEO_H__
#define __YAK_VIDEO_H__

#include <yak/video/vbe.h>

struct renderer;

struct surface
{
    volatile unsigned int x, y; /* position within the surface */
    unsigned int w, h; /* with and height of surface */
    unsigned int ox, oy; /* position of surface within fb */
    unsigned int fb_w, fb_h; /* width and height of fb */
    unsigned int fb_bpp; /* bits per pixel */
    unsigned int fg_color, bg_color;
    unsigned int scroll;
    unsigned char *framebuffer, *backbuffer;
    struct renderer *rend;
};

struct renderer
{
    void (*scroll)(struct surface *surf);
    void (*clear)(struct surface *surf);
    void (*putc)(struct surface *surf, char c);
};

void surface_init(struct surface *surf);

#endif
