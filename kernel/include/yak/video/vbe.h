#ifndef __VBE_H__
#define __VBE_H__

#include <yak/lib/types.h>

typedef struct
{
    union {
        i8_t  sig_chr[4];
        u32_t sig_32;
    } vbe_signature;
    u16_t vbe_version;
    u32_t oem_string_ptr;
    u8_t  capabilities[4];
    u32_t video_mode_ptr;
    u16_t total_memory;
    u16_t oem_software_rev;
    u32_t oem_vendor_name_ptr;
    u32_t oem_product_name_ptr;
    u32_t oem_product_rev_ptr;
    u8_t  reserved[222];
    u8_t  oem_data[256];
} __attribute__((packed)) vbe_info_t;

typedef struct
{
    u16_t attributes;
    u8_t  windowA, windowB;
    u16_t granularity;
    u16_t window_size;
    u16_t segmentA, segmentB;
    u32_t window_func_ptr;
    u16_t pitch;
    u16_t res_x, res_y;
    u8_t  wchar, ychar, planes, bpp, banks;
    u8_t  memory_model, bank_size, image_pages;
    u8_t  reserved0;
    u8_t  red_mask, red_pos;
    u8_t  green_mask, green_pos;
    u8_t  blue_mask, blue_pos;
    u8_t  reserved_mask, reserved_pos;
    u8_t  direct_color_attributes;
    u32_t phys_base;
    u32_t off_screen_mem_off;
    u16_t off_screen_mem_size;
    u8_t  reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;

#endif
