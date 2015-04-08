CC = i686-elf-gcc
AS = i686-elf-as
CPP = i686-elf-cpp
LD = i686-elf-ld

CFLAGS = -ffreestanding -fbuiltin -Wall -Wextra -Werror -fno-tree-loop-optimize -std=gnu11
AFLAGS = 
LDFLAGS = -static
LIBS = -nostdlib

STAGE1_OBJ = stage1.o 
STAGE2_OBJ = stage2.o
STAGE1_BIN = stage1.bin
STAGE2_BIN = stage2.bin
INC = /home/jo/dev/yak-64/sysroot/usr/include
DISK = ../disk.img

all: $(STAGE1_BIN) $(STAGE2_BIN) 

.PHONY: all clean install

$(STAGE1_BIN): stage1.S
	@echo "[GAS]\t$@"
	$(CPP) -I $(INC) -P $< -o stage1.s
	$(AS) $(AFLAGS) -o $(STAGE1_OBJ) stage1.s
	$(LD) -Ttext 0x7c00 --oformat binary -o $@ $(STAGE1_OBJ) 

$(STAGE2_BIN): stage2.c
	@echo "[CC]\t$@"
	$(CC) $(CLFAGS) -std=gnu11 -I $(INC) -c $< -o $(STAGE2_OBJ)
	$(LD) $(LDFLAGS) $(LIBS) -T stage2.ld --oformat binary -o $@ $(STAGE2_OBJ)

clean:
	@rm -fv *.o *.bin $(DISK) stage1.s

#install: install-headers install-mbr

install-headers:

install: $(STAGE1_BIN) $(STAGE2_BIN) $(DISK)
	@echo "[MBR]"
	# copy jump and nop
	@dd if=$(STAGE1_BIN) of=$(DISK) bs=1 count=3 skip=0 seek=0 conv=notrunc
	# copy code, don't overwrite BPB and partition table
	@dd if=$(STAGE1_BIN) of=$(DISK) bs=1 count=356 skip=90 seek=90 conv=notrunc
	# copy signature
	@dd if=$(STAGE1_BIN) of=$(DISK) bs=1 count=2 skip=510 seek=510 conv=notrunc
	# backup sector
	@dd if=$(DISK) of=$(DISK) bs=512 count=1 skip=0 seek=20 conv=notrunc
	# stage2 will begin at sector 2, after the FSinfo sector
	@dd if=$(STAGE2_BIN) of=$(DISK) bs=512 seek=2 conv=notrunc

$(DISK):
	@echo "[DISK]"
	@rm -f $(DISK)
	@mkdosfs -C -v -F32 -f 2 -R 32 -b 20 -n YAK $(DISK) 64000
