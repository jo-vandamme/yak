.code16

#include "boot.h"

.global _start
_start:
    jmp begin
    nop

# BPB takes 79 bytes from here
. = _start + BPB_START
.space 79

# Bootloader data area
. = _start + BOOT_DATA_AREA
stage2_load_address:    .word STAGE2_LOAD_ADDRESS
stage2_sector_start:    .long 2
stage2_num_sectors:     .long 20

begin:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw $0x7c00, %bp
    movw $(STAGE2_LOAD_ADDRESS >> 4), %ax
    movw %ax, %es
    movw $0x7bfc, %sp
    sti
    ljmp $0, $load_cs

load_cs:
    cld
    testb $0x7f, %dl        # force BIOS drive number to a sane value
    jz dl_valid
    movb BPB_DRIVE_NUMBER(%bp), %dl  # if it is bad, get it from the data area
dl_valid:
    movb %dl, BPB_DRIVE_NUMBER(%bp)  # save the drive number
    testb %dl, %dl
    jns load_kernel         # avoid this call on floppy devices
    movb $8, %ah
    int $0x13               # get the disk geometry from the BIOS
    jc load_kernel
    andw $0x3f, %cx
    movw %cx, BPB_SECTORS_PER_TRACK(%bp)
    movb %dh, %cl
    incw %cx
    movw %cx, BPB_NUMBER_OF_HEADS(%bp)

    # clear the screen
    movw $0x0003, %ax
    int $0x10

    movw $welcome_msg, %si
    call print

load_kernel:
    xorw %bx, %bx           # offset of buffer
    movw stage2_sector_start, %ax
    movw stage2_num_sectors, %cx

sector_loop:
    call read_sector        # read sector
    addw BPB_BYTES_PER_SECTOR(%bp), %bx  # increment buffer
    incw %ax                # increment LBA
    loop sector_loop        # loop until all sectors are read

execute:
    jmp STAGE2_LOAD_ADDRESS
    call reboot

# read sector # ax into es:bx
read_sector:
    pusha
    movw $5, %di
loop:
    pushw %ax               # save LBA address
    xorw %dx, %dx           # remainder of division will go in dx (modulo)
    divw BPB_SECTORS_PER_TRACK(%bp) # ax = lba / sectors per track, dx = remainder
    incb %dl                # dx = lba % sectors per track + 1
    movb %dl, %cl           # sector = (lba MOD sectors per track) + 1
    xorw %dx, %dx           # remainder of division will go in dx (modulo)
    divw BPB_NUMBER_OF_HEADS(%bp)
    movb %al, %ch           # track = lba / (sectors per track * number of heads)
    movb %dl, %dh           # head = (lba / sectors per track) MOD number of heads
    movb BPB_DRIVE_NUMBER(%bp), %dl # drive number
    movw $0x0201, %ax       # ah = 2 (function #) al = 1 (# of sectors)
    int $0x13
    jnc success
    xorw %ax, %ax           # BIOS reset disk
    int $0x13
    decw %di                # decrement error counter
    popw %ax                # restore LBA address
    jnz loop
    call reboot
success:
    movw $progress_msg, %si
    call print
    popw %ax
    popa
    ret

# print an error message and reboot after keystroke
reboot:
    movw $death_msg, %si
    call print
    movb $0, %ah
    int $0x16               # wait for keystroke
    .byte 0xea
    .word 0x0000
    .word 0xffff

# print a null terminated string from ds:si
# modifies ax and bx
print:
    lodsb                   # load next byte from si to al
    orb %al, %al
    jz 1f
    movb $0x0e, %ah
    movw $0x0007, %bx       # bh = 0, bl = 0x07
    int $0x10
    jmp print
1:
    ret

welcome_msg:    .asciz "Stage1"
progress_msg:   .asciz "."
death_msg:      .asciz "Error: press a key to reboot"

# Partition table takes 63 bytes from here
. = _start + PARTITION_TABLE
.space 63

# Boot signature
. = _start + 0x1fe
.short 0xaa55
