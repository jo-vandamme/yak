#ifndef __ELF_H__
#define __ELF_H__

#include <yak/lib/types.h>
#include "common.h"

typedef struct {
    u8_t     ident[16];
    u16_t    type;
    u16_t    machine;
    u32_t    version;
    u64_t    entry;
    u64_t    phoff;
    u64_t    shoff;
    u32_t    flags;
    u16_t    ehsize;
    u16_t    phentsize;
    u16_t    phnum;
    u16_t    shentsize;
    u16_t    shnum;
    u16_t    shstrndx;
} elf_hdr_t;

typedef struct {
    u32_t    type;
    u32_t    flags;
    u64_t    off;
    u64_t    vaddr;
    u64_t    paddr;
    u64_t    filesz;
    u64_t    memsz;
    u64_t    align;
} prog_hdr_t;

enum elf_ident_t {
    EI_CLASS        = 4, // Architecture (32/64)
    EI_DATA         = 5, // Byte order
    EI_VERSION      = 6, // ELF version
    EI_OSABI        = 7, // OS specific
    EI_ABIVERSION   = 8, // OS specific
    EI_PAD          = 9  // Padding
};

enum elf_type_t {
    ET_NONE         = 0, // Unknown type
    ET_REL          = 1, // Relocatable file
    ET_EXEC         = 2  // Executable file
};

#define ELFDATA2LSB         1 // Little endian
#define ELFCLASS64          2 // 64-bit architecture
#define EM_X86_64          62 // x86 machine type
#define EV_CURRENT          1 // ELF current version
#define PT_LOAD             1

int elf_check_supported(elf_hdr_t *elf)
{
    if (elf->ident[0] != 0x7f && strncmp((const char *)(elf->ident + 1), "ELF", 3) != 0) {
        print("Invalid helf header\r\n");
        return 1;
    }
    if (elf->ident[EI_CLASS] != ELFCLASS64) {
        print("Unsupported ELF file class\r\n");
        return 2;
    }
    if (elf->ident[EI_DATA] != ELFDATA2LSB) {
        print("Unsupported ELF file byte order\r\n");
        return 3;
    }
    if (elf->machine != EM_X86_64) {
        print("Unsupported ELF file target\r\n");
        return 4;
    }
    if (elf->ident[EI_VERSION] != EV_CURRENT) {
        print("Unsupported ELF file version\r\n");
        return 5;
    }
    if (elf->type != ET_REL && elf->type != ET_EXEC) {
        print("Unsupported ELF file type\r\n");
        return 6;
    }
    return 0;
}

#endif
