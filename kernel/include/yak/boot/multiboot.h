#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__

#include <yak/lib/types.h>

#define MBOOT_KERNEL_MAGIC 0x1badb002
#define MBOOT_LOADER_MAGIC 0x2badb002

#define MBOOT_FLAGS_PAGE_ALIGN  (1 << 0)
#define MBOOT_FLAGS_MMAP        (1 << 1)
#define MBOOT_FLAGS_VIDEO       (1 << 2)
#define MBOOT_FLAGS_LOAD        (1 << 16)

#define MBOOT_MEMINFO       (1 << 0)
#define MBOOT_BOOTDEV       (1 << 1)
#define MBOOT_MODS          (1 << 3)
#define MBOOT_MMAP          (1 << 6)
#define MBOOT_LOADER        (1 << 9)
#define MBOOT_VBE           (1 << 11)

typedef struct
{
    u32_t magic;
    u32_t flags;
    u32_t checksum;
    u32_t header_addr;       // if flags[16] is set
    u32_t load_addr;         // if flags[16] is set
    u32_t load_end_addr;     // if flags[16] is set
    u32_t bss_end_addr;      // if flags[16] is set
    u32_t entry_addr;        // if flags[16] is set
    u32_t mode_type;         // if flags[2] is set
    u32_t width;             // if flags[2] is set
    u32_t height;            // if flags[2] is set
    u32_t depth;             // if flags[2] is set
} __attribute__((packed)) multiboot_header_t;

typedef struct
{
    u32_t flags;
    u32_t mem_lower;         // if flags[0] is set
    u32_t mem_upper;         // if flags[0] is set
    u32_t boot_device;       // if flags[1] is set
    u32_t cmd_line;          // if flags[2] is set
    u32_t mods_count;        // if flags[3] is set
    u32_t mods_addr;         // if flags[3] is set
    u32_t num;               // if flags[4] or flags[5] is set
    u32_t size;              // if flags[4] or flags[5] is set
    u32_t addr;              // if flags[4] or flags[5] is set
    u32_t shndx;             // if flags[4] or flags[5] is set
    u32_t mmap_length;       // if flags[6] is set
    u32_t mmap_addr;         // if flags[6] is set
    u32_t drives_length;     // if flags[7] is set
    u32_t drives_addr;       // if flags[7] is set
    u32_t config_table;      // if flags[8] is set
    u32_t boot_loader_name;  // if flags[9] is set
    u32_t apm_table;         // if flags[10] is set
    u32_t vbe_control_info;  // if flags[11] is set
    u32_t vbe_mode_info;
    u16_t vbe_mode;
    u16_t vbe_interface_seg;
    u16_t vbe_interface_off;
    u16_t vbe_interface_len;
} __attribute__((packed)) multiboot_info_t;

typedef struct
{
    u32_t size;
    u64_t addr;
    u64_t length;
    u32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

#endif
