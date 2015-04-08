asm(".code16gcc\n"
    ".global _start\n"
    "_start:\n"
    "jmpl $0, $main");

#include "common.h"
#include "elf.h"
#include "mmap.h"
#include "cpuid.h"
#include "boot.h"

#include <yak/lib/types.h>
#include <yak/boot/multiboot.h>
#include <yak/video/vbe.h>

#define MBOOT_LOC           0x0d00 // 0x0058 bytes
#define LOADER_NAME_LOC     0x0d60 // 0x0040 bytes
#define VBE_INFO_LOC        0x0e00 // 0x0200 bytes
#define VBE_MODE_INFO_LOC   0x1000 // 0x0100 bytes
#define MMAP_LOC            0x1100 // 0x2000 bytes
#define MMAP_LOC_TEMP       0x3000
#define MMAP_SIZE           0x2000

u32_t sec_per_fat, data_start, root_cluster;
u16_t res_sectors, sec_per_track, num_heads, bytes_per_sector;
u8_t fat_count, sec_per_cluster, drive;

multiboot_info_t *mboot = (multiboot_info_t *)MBOOT_LOC;
vbe_info_t *vbe_info = (vbe_info_t *)VBE_INFO_LOC;
vbe_mode_info_t *mode_info;

void read_sector(u32_t lba, u32_t offset);
u32_t cluster_to_lba(u32_t c);
u32_t fat_next_cluster(u32_t cluster);
void enable_a20_line(void);
u8_t test_a20_line(void);

#define VBE_SUCCESS 0x004f
u16_t vbe_set_mode(u16_t mode);
u16_t find_vbe_mode(vbe_info_t *vbe_info, int x, int y, int d);
u16_t vbe_get_mode_info(vbe_mode_info_t *mode_info, u16_t mode);
u16_t vbe_get_info(vbe_info_t *vbe_info);

typedef struct
{
    u16_t limit_low;
    u16_t base_low;
    u8_t  base_middle;
    u8_t  access;
    u8_t  granularity;
    u8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct
{
    u16_t limit;
    u32_t base;
} __attribute__((packed)) gdt_ptr_t;

gdt_entry_t gdt[3] __attribute__((aligned(16))) = {
    { /* null segment */
        .limit_low = 0, 
        .base_low = 0, 
        .base_middle = 0, 
        .access = 0, 
        .granularity = 0, 
        .base_high = 0 
    },
    { /* code segment */
        .limit_low = 0xffff, 
        .base_low = 0, 
        .base_middle = 0, 
        .access = 0x9a, 
        .granularity = 0xcf, 
        .base_high = 0 
    },
    { /* data segment */
        .limit_low = 0xffff, 
        .base_low = 0, 
        .base_middle = 0, 
        .access = 0x92, 
        .granularity = 0xcf, 
        .base_high = 0 
    }
};

gdt_ptr_t gdt_ptr = {
    .limit = sizeof(gdt) - 1,
    .base = (u32_t)&gdt
};

#define END_OF_CLUSTER 0x0ffffff8

void main(void)
{
    // Switch to unreal mode
    asm volatile(
            "cli\n"
            "xorw %%ax, %%ax\n"
            "movw %%ax, %%ds\n"
            "movw %%ax, %%es\n"
            "movw %%ax, %%fs\n"
            "movw %%ax, %%gs\n"
            "lgdt (%0)\n"         // load GDT
            "movl %%cr0, %%eax\n"
            "or   $0x01, %%al\n"  // switch to pmode
            "movl %%eax, %%cr0\n"
            "jmp .+2\n"
            "movw  $0x10, %%ax\n" // load a selector
            "movw  %%ax, %%ds\n"
            "movw  %%ax, %%es\n"
            "movw  %%ax, %%ss\n"
            "movl %%cr0, %%eax\n"
            "and  $0xfe, %%al\n"  // switch back to unreal mode
            "movl %%eax, %%cr0\n"
            "xorw %%ax, %%ax\n"
            "movw %%ax, %%ds\n"
            "movw %%ax, %%es\n"
            "movw %%ax, %%ss\n"
            "movw $0x7c00, %%sp\n"
            "sti\n"
            : : "m"(gdt_ptr) : "cc", "memory", "eax");

    print("\r\nStage2");

    // Enable the A20 line
    if (!test_a20_line()) {
        enable_a20_line();
        if (!test_a20_line()) {
            print("Can't enable A20 line\n");
            goto error;
        }
    }

    bytes_per_sector = *(u16_t *)(BPB_BYTES_PER_SECTOR + STAGE1_BASE);
    sec_per_fat = *(u32_t *)(BPB_SECTORS_PER_FAT32 + STAGE1_BASE);
    fat_count = *(u8_t *)(BPB_FAT_COUNT + STAGE1_BASE);
    res_sectors = *(u16_t *)(BPB_RESERVED_SECTORS + STAGE1_BASE);
    root_cluster = *(u32_t *)(BPB_ROOT_CLUSTER + STAGE1_BASE);
    sec_per_cluster = *(u8_t *)(BPB_SECTORS_PER_CLUSTER + STAGE1_BASE);
    sec_per_track = *(u16_t *)(BPB_SECTORS_PER_TRACK + STAGE1_BASE);
    num_heads = *(u16_t *)(BPB_NUMBER_OF_HEADS + STAGE1_BASE);
    drive = *(u8_t *)(BPB_DRIVE_NUMBER + STAGE1_BASE);

    data_start = res_sectors + fat_count * sec_per_fat;

    u32_t dir_entry, cluster = root_cluster;
    int found = 0;

    // ------------------------------------------
    // Verify if CPUID and Longmode are supported
    // ------------------------------------------

    if (!has_cpuid()) {
        print("\r\nCPUID not supported\r\n");
        goto error;
    }

    if (!has_longmode()) {
        print("\r\nLongmode not supported\r\n");
        goto error;
    }

    // -----------------------------------------
    // Parse the FAT32 filesystem for the kernel
    // -----------------------------------------
    for (;;) {
        read_sector(cluster_to_lba(cluster), BUFFER);
        for (dir_entry = BUFFER; dir_entry < BUFFER + bytes_per_sector; dir_entry += 32) {
            //asm volatile(" " ::: "memory"); // XXX strange bug without this...
            if (strncmp("YAK     ELF", (const char *)dir_entry, 11) == 0) {
                found = 1;
                break;
            }
        }
        if (found || cluster >= END_OF_CLUSTER)
            break;
        cluster = fat_next_cluster(cluster);
    }
    if (!found) {
        print("\r\nError: kernel not found!\r\n");
        goto error;
    }

    // -------------------------------------
    // Load the kernel to KERNEL_LOW_ADDRESS
    // -------------------------------------
    cluster = *(u16_t *)(dir_entry + 0x14) << 8 | 
              *(u16_t *)(dir_entry + 0x1a);

    u8_t *kernel_base = (u8_t *)KERNEL_LOW_ADDRESS;
    unsigned int i;
    do {
        read_sector(cluster_to_lba(cluster), BUFFER);
        for (i = 0; i < bytes_per_sector; ++i) {
            *(kernel_base++) = *(u8_t *)(BUFFER + i);
        }
        put_char('.');
        cluster = fat_next_cluster(cluster);
    } while (cluster < END_OF_CLUSTER);
    print("\r\n");

    // ---------------------------------------------------------------
    // Parse the ELF64 kernel image and move it to KERNEL_HIGH_ADDRESS
    // ---------------------------------------------------------------

    elf_hdr_t *elf = (elf_hdr_t *)KERNEL_LOW_ADDRESS;
    if (elf_check_supported(elf)) {
        print("Can't load ELF kernel");
        goto error;
    }
    prog_hdr_t *prog_hdr = (prog_hdr_t *)((u8_t *)elf + elf->phoff);
    u8_t *paddr, *image;
    for (unsigned i = 0; i < elf->phnum; ++i) {

#ifdef _DEBUG_
        print("Type:  "); print_num(prog_hdr[i].type, 8); print("\r\n");
        print("Flags: "); print_num(prog_hdr[i].flags, 8); print("\r\n");
        print("Offset:"); print_num(prog_hdr[i].off, 16); print(" ");
        print("Virt:  "); print_num(prog_hdr[i].vaddr, 16); print(" ");
        print("Phys:  "); print_num(prog_hdr[i].paddr, 16); print("\r\n");
        print("Filesz:"); print_num(prog_hdr[i].filesz, 16); print(" ");
        print("Memsz: "); print_num(prog_hdr[i].memsz, 16); print(" ");
        print("Align: "); print_num(prog_hdr[i].align, 16); print("\r\n");
#endif

        if (prog_hdr[i].type != PT_LOAD) 
            continue;

        paddr = (u8_t *)((u32_t)prog_hdr[i].paddr);
        image = (u8_t *)((u32_t)(prog_hdr[i].off + (u32_t)elf));

        u32_t size = prog_hdr[i].filesz;
        while (size-- > 0) {
            *paddr++ = *image++;
        }
        memset((void *)paddr, 0, prog_hdr[i].memsz - prog_hdr[i].filesz);
    }

    // ----------------------------------------
    // Find and initialize the Multiboot header
    // ----------------------------------------
    u32_t *mem = (u32_t *)(KERNEL_LOW_ADDRESS - KERNEL_LOW_ADDRESS % 4);
    for (unsigned i = 0; i < 8192; ++i, ++mem) {
        if (*mem == MBOOT_KERNEL_MAGIC) {
            break;
        }
    }
    multiboot_header_t *mboot_hdr = (multiboot_header_t *)mem;
    if (mboot_hdr->magic != MBOOT_KERNEL_MAGIC) {
        print("Multiboot magic not found\r\n");
        goto error;
    }
    if (mboot_hdr->checksum + mboot_hdr->magic + mboot_hdr->flags != 0) {
        print("Multiboot checksum mismatch\r\n");
        goto error;
    }
    
    // Initialize multiboot info structure passed to the kernel
    memset((void *)mboot, 0, sizeof(multiboot_info_t));

    const char *name = "FAT32 loader";
    char *loader_name = (char *)LOADER_NAME_LOC;
    while (*name) {
        *loader_name++ = *name++;
    }
    *loader_name = '\0';
    mboot->boot_loader_name = LOADER_NAME_LOC;
    mboot->flags |= MBOOT_LOADER;

    mboot->boot_device = drive;
    mboot->flags |= MBOOT_BOOTDEV;

    // -----------------------
    // Gather e820 memory info
    // -----------------------
    if (mboot_hdr->flags & MBOOT_FLAGS_MMAP) {
        mem_map_t *mmap = (mem_map_t *)MMAP_LOC_TEMP;
        const int mmap_size = MMAP_SIZE;
        int entries = detect_memory(mmap, mmap_size / sizeof(mem_map_t));
        if (entries == -1) {
            print("Error, can't make a memory map\r\n");
            goto error;
        }
#ifdef _DEBUG
        print_memory_map(mmap, entries); 
#endif

        multiboot_mmap_entry_t *mboot_mmap = (multiboot_mmap_entry_t *)MMAP_LOC;
        for (int i = 0; i < entries; ++i) {
            mboot_mmap->size = sizeof(multiboot_mmap_entry_t) - sizeof(u32_t);
            mboot_mmap->addr = mmap[i].base;
            mboot_mmap->length = mmap[i].length;
            mboot_mmap->type = mmap[i].type;
            mboot_mmap++;
        }
        mboot->mmap_addr = MMAP_LOC;
        mboot->mmap_length = entries * sizeof(multiboot_mmap_entry_t);
        mboot->flags |= MBOOT_MMAP;
    }

    // ---------------
    // Set a VESA mode
    // ---------------
    if (mboot_hdr->flags & MBOOT_FLAGS_VIDEO) {
        mode_info = (vbe_mode_info_t *)VBE_MODE_INFO_LOC;
        vbe_info = (vbe_info_t *)VBE_INFO_LOC;
    
        // get controller info
        if (vbe_get_info(vbe_info) != VBE_SUCCESS) {
            print("VBE get info error\r\n");
            goto error;
        }
        if (strncmp("VESA", (const char *)vbe_info->vbe_signature.sig_chr, 4)) {
            print("Invalid VBE block info\r\n");
            goto error;
        }
        // find appropriate mode
        int clear_video = 1;
        u16_t mode = 0;
        if ((mode = find_vbe_mode(
                        vbe_info, mboot_hdr->width, mboot_hdr->height, mboot_hdr->depth)) == 0) {
            print("No VBE mode found\r\n");
            goto error;
        }
        if (vbe_get_mode_info(mode_info, mode) != VBE_SUCCESS) {
            print("Can't get VBE mode information\r\n");
            goto error;
        // set mode - 14th bit set for LFB - 15th bit set to keep video memory
        } else if (vbe_set_mode(
                    mode | mboot_hdr->mode_type << 14 | !clear_video << 15) != VBE_SUCCESS) {
            asm volatile("int $0x10" : : "a"(0x0003));
            print("Can't set VBE mode\n");
            goto error;
        } else {
            mboot->vbe_control_info = (u32_t)vbe_info;
            mboot->vbe_mode_info = (u32_t)mode_info;
            mboot->vbe_mode = mode;
            mboot->flags |= MBOOT_VBE;
        }
    }
    //putpixel2(200, 200, 0xff0000);

    // -----------------------------------------------
    // switch to protected mode and execute the kernel
    // -----------------------------------------------
    asm volatile(
        "cli\n"
        "movl %%cr0, %%eax\n"
        "andl $0x7fffffff, %%eax\n" // clear bit 31 (PG)
        "or $0x1, %%al\n"           // set bit 0 (PE)
        "movl %%eax, %%cr0\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"
        "movl %0, (.jumping + 2)\n" // this overwrites the 0 below and
        "movl %1, %%eax\n"
        "movl %2, %%ebx\n"
        ".jumping:\n"               // sets the address to jump to
        "data32 ljmp $0x08, $0\n"
        : : "q"(elf->entry), "i"(MBOOT_LOADER_MAGIC), "i"(MBOOT_LOC)
        : "eax", "ebx", "memory", "cc");

    // restore text mode
    asm volatile("int $0x10" : : "a"(0x0003));

error:
    print("An error occured, restart the computer\r\n");
    for (;;) { }
}

u32_t cluster_to_lba(u32_t c)
{
    return (c - 2) * sec_per_cluster + data_start;
}

u32_t fat_next_cluster(u32_t cluster)
{
    // 32 bytes per cluster number, we read the last fat, just before the data
    u32_t offset = (cluster * 4) % bytes_per_sector;
    u32_t sector = (cluster * 4) / bytes_per_sector;

    read_sector(data_start - fat_count * sec_per_fat + sector, BUFFER);
    u32_t next = *(u32_t *)(BUFFER + offset) & 0x0fffffff;

    return next;
}

void read_sector(u32_t lba, u32_t offset)
{

    u8_t sector, track, head;
    sector = (lba % sec_per_track) + 1;
    track = lba / (sec_per_track * num_heads);
    head = (lba / sec_per_track) % num_heads;

    u16_t status;
    asm volatile("int $0x13" 
                 : "=a"(status)
                 : "a"(0x0201), "b"(offset), 
                   "c"(track << 8 | sector), "d"(head << 8 | drive)
                 : );
}

void enable_a20_line(void)
{
    /* enable A20 line via the BIOS */
    asm volatile("mov $0x2401, %ax\n"
                 "int $0x15");
}

u8_t test_a20_line(void)
{
    u8_t status;
    asm volatile("mov $0x2402, %%ax\n"
                 "int $0x15"
                 : "=a"(status));
    return status;
}

int strncmp(const char *s1, const char *s2, unsigned int n)
{
    for (; n > 0; s1++, s2++, --n) {
        if (*s1 != *s2)
            return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : 1);
        else if (*s1 == '\0')
            return 0;
    }
    return 0;
}

void* memset(void* bufptr, int value, u32_t size)
{
    unsigned char* buf = (unsigned char*)bufptr;
    for (u32_t i = 0; i < size; ++i) 
        buf[i] = (unsigned char)value;
    return bufptr;
}

void put_char(const char c)
{
    asm volatile("int $0x10" : : "a"(0x0e00 | c), "b"(7) : "memory");
}

void print(const char *str)
{
    while (*str)
        put_char(*str++);
}

#ifdef _DEBUG_

void print_num(unsigned long long num, int size)
{
    char tmp[17] = { 0 };
    int i;

    if (size > 16)
        size = 16;
    for (i = 0; num > 0 && i < size; ++i) {
        char c = (char)(num & 0xf);
        if (c > 9)
            tmp[i] = c - 10 + 'a';
        else
            tmp[i] = c + '0';
        num /= 16;
    }
    for (i = size-1; i >= 0; --i) {
        if (!tmp[i])
            put_char('0');
        else
            put_char(tmp[i]);
    }
}

void show_mem(const char *start, int size, int nperline)
{
    int i;
    for (i = 0; i < size; ++i) {
        if (i % nperline == 0) {
            print("\r\n");
            print_num(i, 3);
            print(": ");
        } else {
            put_char(' ');
        }
        print_num(*(unsigned int *)(start + i), 2);
    }
}

#endif

u16_t vbe_get_info(vbe_info_t *vbe_info)
{
    for (unsigned i = 0; i < sizeof(vbe_info_t); ++i)
        *((u8_t *)vbe_info + i) = 0;

    *((char *)vbe_info + 0) = 'V';
    *((char *)vbe_info + 1) = 'B';
    *((char *)vbe_info + 2) = 'E';
    *((char *)vbe_info + 3) = '2';
        
    u16_t status = 0x4f00;
    asm volatile("int $0x10" 
                 : "+a"(status)
                 : "D"(vbe_info) 
                 : "memory", "cc");
    return status;
}

u16_t vbe_get_mode_info(vbe_mode_info_t *mode_info, u16_t mode)
{
    u16_t status = 0x4f01;
    asm volatile("int $0x10" 
                 : "+a"(status)
                 : "c"(mode), "D"(mode_info) 
                 : "memory", "cc");
    return status;
}

u16_t vbe_set_mode(u16_t mode)
{
    u16_t status = 0x4f02;
    asm volatile("int $0x10" 
                 : "+a"(status) 
                 : "b"(mode)
                 : "memory", "cc");
    return status;
}

#define abs(x) ((x) < 0 ? -(x) : (x))

u16_t find_vbe_mode(vbe_info_t *vbe_info, int x, int y, int d)
{
    u16_t best = 0x13;
    int area = x * y;
    if (area == 0) {
        area = 4000 * 4000;
        d = 32;
    }
    int pix_diff, best_pix_diff = abs(320 * 200 - area);
    int depth_diff, best_depth_diff = 8 >= d ? 8 - d : (d - 8) * 2;

    u16_t *modes = (u16_t *)vbe_info->video_mode_ptr;
    vbe_mode_info_t mode_info;

    for (int i = 0; modes[i] != 0xffff; ++i) {
        if (vbe_get_mode_info(&mode_info, modes[i]) != VBE_SUCCESS) continue; 
        /* check if this is a graphics mode with LFB support and it is supported by hardware */
        if ((mode_info.attributes & 0x91) != 0x91) continue;

        /* check if this is a packed pixel or a direct color mode */
        if (mode_info.memory_model != 4 && mode_info.memory_model != 6) continue;

        /* exact match */
        if (x == mode_info.res_x && y == mode_info.res_y && d == mode_info.bpp)
            return modes[i];

        pix_diff = abs(mode_info.res_x * mode_info.res_y - area);
        depth_diff = (mode_info.bpp >= d) ? mode_info.bpp - d : (d - mode_info.bpp) * 2;
        if (pix_diff < best_pix_diff ||
           (pix_diff == best_pix_diff && depth_diff < best_depth_diff)) {
            best = modes[i];
            best_pix_diff = pix_diff;
            best_depth_diff = depth_diff;
        }
    }
    if (x == 640 && y == 480 && d == 1) return 0x11;
    return best;
}

#ifdef _DEBUG_

u16_t cur_bank = 0;

void vbe_bank_switch(u16_t bank)
{
    if (bank == cur_bank) 
        return;

    u16_t status1 = 0x4f05;
    asm volatile("int $0x10" 
                 : "+a"(status1) 
                 : "b"(0), "d"(bank)
                 : "memory", "cc");

    u16_t status2 = 0x4f05;
    asm volatile("int $0x10" 
                 : "+a"(status2)
                 : "b"(1), "d"(bank)
                 : "memory", "cc");
    status1 &= 0xff00;
    status2 &= 0xff00;

    if (!status1 && !status2)
        cur_bank = bank;
}

void putpixel(u32_t x, u32_t y, u32_t color)
{
    u8_t *video = (u8_t *)mode_info->phys_base;
    u32_t where = mode_info->bpp / 8 * x + y * mode_info->pitch;
    video[where] = color & 0xff;
    if (mode_info->bpp == 16) {
        video[where + 1] = (color >> 8) & 0xff;
    } else if (mode_info->bpp == 24) {
        video[where + 1] = (color >> 8) & 0xff;
        video[where + 2] = (color >> 16) & 0xff;
    }
}

void putpixel2(u32_t x, u32_t y, u32_t color)
{
    u8_t *fb = (u8_t *)0xa0000;
    u32_t offset = x*3 + y * (u32_t)mode_info->pitch;
    u32_t bank_size = mode_info->granularity * 1024;

    vbe_bank_switch((offset + 0) / bank_size);
    fb[(offset + 0) % bank_size] = (color >> 0)  & 0xff;

    vbe_bank_switch((offset + 1) / bank_size);
    fb[(offset + 1) % bank_size] = (color >> 8)  & 0xff;

    vbe_bank_switch((offset + 2) / bank_size);
    fb[(offset + 2) % bank_size] = (color >> 16) & 0xff;
}

#endif
