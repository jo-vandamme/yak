#ifndef __YAK_ACPI_H__
#define __YAK_ACPI_H__

#include <yak/lib/types.h>

typedef struct
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) sdt_header_t;

uintptr_t acpi_init(void);

#endif
