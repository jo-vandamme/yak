#ifndef __BOOT_H__
#define __BOOT_H__

//#define _DEBUG_

#define BPB_START               0x03
#define BPB_BYTES_PER_SECTOR    0x0b
#define BPB_SECTORS_PER_CLUSTER 0x0d
#define BPB_RESERVED_SECTORS    0x0e
#define BPB_FAT_COUNT           0x10
#define BPB_SECTORS_PER_TRACK   0x18
#define BPB_NUMBER_OF_HEADS     0x1a
#define BPB_SECTORS_PER_FAT32   0x24
#define BPB_ROOT_CLUSTER        0x2c
#define BPB_DRIVE_NUMBER        0x40

#define BOOT_DATA_AREA          0x5a
#define BDA_ST2_LOAD_ADDRESS    0x5a
#define BDA_ST2_SECTOR_START    0x5c
#define BDA_ST2_NUM_SECTORS     0x60

#define PARTITION_TABLE         0x1be

#define STAGE1_BASE             0x0007c00 // 1 sector
#define BUFFER                  0x0007e00 // 1 sector
#define STAGE2_LOAD_ADDRESS     0x0008000 // 20 sectors
#define KERNEL_LOW_ADDRESS      0x0010000
#define KERNEL_HIGH_ADDRESS     0x0100000

#endif
