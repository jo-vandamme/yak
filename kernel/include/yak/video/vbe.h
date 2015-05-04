#ifndef __VBE_H__
#define __VBE_H__

#include <yak/lib/types.h>

typedef struct
{
    union {
        char  sig_chr[4];
        uint32_t sig_32;
    } vbe_signature;
    uint16_t vbe_version;
    uint32_t oem_string_ptr;
    uint8_t  capabilities[4];
    uint32_t video_mode_ptr;
    uint16_t total_memory;
    uint16_t oem_software_rev;
    uint32_t oem_vendor_name_ptr;
    uint32_t oem_product_name_ptr;
    uint32_t oem_product_rev_ptr;
    uint8_t  reserved[222];
    uint8_t  oem_data[256];
} __attribute__((packed)) vbe_info_t;

typedef struct
{
    uint16_t attributes;
    uint8_t  windowA, windowB;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segmentA, segmentB;
    uint32_t window_func_ptr;
    uint16_t pitch;
    uint16_t res_x, res_y;
    uint8_t  wchar, ychar, planes, bpp, banks;
    uint8_t  memory_model, bank_size, image_pages;
    uint8_t  reserved0;
    uint8_t  red_mask, red_pos;
    uint8_t  green_mask, green_pos;
    uint8_t  blue_mask, blue_pos;
    uint8_t  reserved_mask, reserved_pos;
    uint8_t  direct_color_attributes;
    uint32_t phys_base;
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t  reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;

#endif
