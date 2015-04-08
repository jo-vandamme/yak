#ifndef __MMAP_H__
#define __MMAP_H__

#include "common.h"
#include <yak/lib/types.h>

typedef struct
{
    u64_t base;
    u64_t length;
    u32_t type;
    u32_t acpi;
} __attribute__((packed)) mem_map_t;

#define MMAP_MAGIC 0x534d4150

int detect_memory(mem_map_t *buffer, int max_entries)
{
    u32_t contID = 0;
    int entries = 0, signature, bytes;
    do {
        asm volatile("int $0x15"
                     : "=a"(signature), "=c"(bytes), "=b"(contID)
                     : "a"(0xe820), "b"(contID), "c"(24), "d"(MMAP_MAGIC), "D"(buffer));
        if (signature != MMAP_MAGIC) {
            return -1;
        }
        if (bytes > 20 && (buffer->acpi & 1 << 0) == 0) {
            /* ignore this entry */
        } else {
            ++buffer;
            ++entries;
        }
    } while (contID != 0 && entries < max_entries);

    return entries;
}

#ifdef _DEBUG_

void print_memory_map(mem_map_t *mmap, int entries)
{
    const char *mem_types[] = { "Usable", "Reserved", "ACPI reclaimed", "ACPI NVS", "Bad memory" };

    print("Base             Length           Type\r\n");
    for (int i = 0; i < entries; ++i) {
        if (mmap[i].type != 1) continue;
        print_num(mmap[i].base, 16); print(" ");
        print_num(mmap[i].length, 16); print(" ");
        print(mem_types[(mmap[i].type - 1) % 5]); print("\r\n");
    }
}

#endif

#endif
