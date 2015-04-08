#ifndef __MMAP_H__
#define __MMAP_H__

#include <yak/lib/types.h>

typedef struct
{
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) mmap_entry_t;


#endif
