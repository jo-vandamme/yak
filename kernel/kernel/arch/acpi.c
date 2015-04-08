#include <yak/lib/types.h>
#include <yak/lib/string.h>
#include <yak/config.h>
#include <yak/initcall.h>
#include <yak/video/printk.h>
#include <yak/mem/vmm.h>
#include <yak/mem/mem.h>
#include <yak/arch/acpi.h>

#define logid "\33\x0a\xf0<acpi>\33r"

// Root System Description Pointer
typedef struct
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed)) rsdp_t;

// RSDP fields added since version 2.0
typedef struct
{
    rsdp_t first_part;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) rsdp2_t;

static INIT_CODE int checksum(unsigned char *addr, int len)
{
    int i, sum = 0;
    for (i = 0; i < len; ++i)
        sum += addr[i];
    return sum & 0xff;
}

// search len bytes from physical address for "RSD PTR " signature
static INIT_CODE rsdp_t *search_region_for_rsdp(uintptr_t address, size_t len)
{
    unsigned char *end, *p, *addr;

    addr = (unsigned char *)VMM_P2V(address);
    end = addr + len;

    for (p = addr; p < end; p += 16)
        if (memcmp(p, "RSD PTR ", 8) == 0 && checksum(p, sizeof(rsdp_t)) == 0)
            return (rsdp_t *)p;
    return 0;
}

// The RSDP structure can be located
// - in the first kilobyte of Extended BIOS Data Area
// - in the BIOS ROM address space between 0xe0000 and 0xfffff
// for now we don't map the region since it is already
// identity mapped, this must then be executed before the identity
// mapping is removed
static INIT_CODE rsdp_t *search_rsdp(void)
{
    unsigned char *bda = (unsigned char *)VMM_P2V(0x400);
    uint32_t p;
    rsdp_t *rsdp = 0;

    if ((p = ((bda[0x0f] << 8) | bda[0x0e]) << 4)) {
        if ((rsdp = search_region_for_rsdp(p, 1024)))
            return rsdp;
    } 
    return search_region_for_rsdp(0xe0000, 0xfffff);
}

// search the sdt headers for a specific signature, if found, the checksum is
// computed and if valid, the header is returned
static INIT_CODE uintptr_t search_sdt(uintptr_t rsdt_address, int xsdt, char signature[4])
{
    sdt_header_t *header = (sdt_header_t *)map_temp(rsdt_address);
    int entries = (header->length - sizeof(sdt_header_t)) / (xsdt ? 8 : 4);
    if (header->length > PAGE_SIZE)
        panic("TODO: Map more pages for the RSDT/XSDT structre!\n");

    sdt_header_t *h;
    uintptr_t sdt_ptr, sdt_ptr_loc;
    for (int i = 0; i < entries; ++i) {
        sdt_ptr_loc = (uintptr_t)header + sizeof(sdt_header_t) + i * (xsdt ? 8 : 4);
        sdt_ptr = (uintptr_t)(xsdt ? *(uint64_t *)sdt_ptr_loc : *(uint32_t *)sdt_ptr_loc);

        h = (sdt_header_t *)map_temp(sdt_ptr);
        if (!strncmp(h->signature, signature, 4) && checksum((unsigned char *)h, h->length) == 0)
            return sdt_ptr;
        header = (sdt_header_t *)map_temp(rsdt_address);
    }
    return 0;
}

INIT_CODE uintptr_t acpi_init(void)
{
    uintptr_t madt = 0;

    rsdp_t *rsdp = search_rsdp();
    if (!rsdp) {
        printk("<acpi> No ACPI support detected!\n");
        return madt;
    }
    char oem_id[7] = { 0 };
    memcpy(oem_id, rsdp->oem_id, 6);
    printk("%s RSDP [%08x%08x] - Rev: %u - OEMID: %s\n", logid,
           ((uintptr_t)rsdp - VIRTUAL_BASE) >> 32, 
           ((uintptr_t)rsdp - VIRTUAL_BASE),
           rsdp->revision + 1, oem_id);

    int use_xsdt = 0;
    uintptr_t rsdt_address = (uintptr_t)rsdp->rsdt_address;
    if (rsdp->revision >= 1) {
        rsdp2_t *rsdp2 = (rsdp2_t *)rsdp;
        if (checksum((unsigned char *)rsdp2, sizeof(rsdp2_t)) != 0) {
            printk("<acpi> Invalid RSDP2 checksum!\n");
            return madt;
        }
        use_xsdt = 1;
        rsdt_address = (uintptr_t)rsdp2->xsdt_address;
    }

    sdt_header_t *rsdt = (sdt_header_t *)map_temp(rsdt_address);
    if (checksum((unsigned char *)rsdt, rsdt->length) != 0) {
        printk("<acpi> Bad RSDT checksum\n");
        return madt;
    }
    char oem_table_id[9] = { 0 };
    memcpy(oem_id, rsdt->oem_id, 6);
    memcpy(oem_table_id, rsdt->oem_table_id, 8);
    uint32_t cid = rsdt->creator_id;
    char creator_id[5] = { 0 };
    creator_id[3] = ((cid >> 24) & 0xff);
    creator_id[2] = ((cid >> 16) & 0xff);
    creator_id[1] = ((cid >> 8) & 0xff);
    creator_id[0] = (cid & 0xff);
    printk("%s RSDT [%08x%08x] - Rev: %u - OEMID: %s\n" \
           "       OEM Table ID: %s - OEM Rev: %u - " \
           "Creator ID: %s - Creator Rev: %u\n", logid,
           rsdt_address >> 32, rsdt_address, rsdt->revision, oem_id, oem_table_id, 
           rsdt->oem_revision, creator_id, rsdt->creator_revision);

    madt = search_sdt(rsdt_address, use_xsdt, "APIC");
    if (madt) {
        printk("%s MADT found\n", logid);
    }

    uintptr_t hpet = search_sdt(rsdt_address, use_xsdt, "HPET");
    if (hpet) {
        printk("%s HPET found\n", logid);
    }

    return madt;
}
