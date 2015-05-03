CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -ffreestanding -fbuiltin -mcmodel=kernel -Wall -Wextra -Werror \
	-mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-omit-frame-pointer
CPPFLAGS:=$(CPPFLAGS) -D__KERNEL__ -Iinclude
LDFLAGS:=$(LDFLAGS) -z max-page-size=0x1000 -Xlinker --defsym -Xlinker KERN_BNUM=$$(cat build.num)
LIBS:=$(LIBS) -nostdlib -lgcc

OBJS:=\
	kernel/boot/loader.o \
	kernel/boot/trampoline.o \
	kernel/boot/main.o \
	kernel/video/terminal.o \
	kernel/video/video.o \
	kernel/video/printk.o \
	kernel/lib/string.o \
	kernel/lib/format.o \
	kernel/lib/rand.o \
	kernel/lib/sort.o \
	kernel/lib/stack.o \
	kernel/cpu/idt.o \
	kernel/cpu/isr.o \
	kernel/cpu/registers.o \
	kernel/cpu/interrupt.o \
	kernel/cpu/percpu.o \
	kernel/cpu/mp.o \
	kernel/mem/mem.o \
	kernel/mem/pmm.o \
	kernel/mem/vmm.o \
	kernel/arch/acpi.o \
	kernel/arch/lapic.o \
	kernel/arch/ioapic.o \
	kernel/arch/pic.o \
	kernel/arch/pit.o \
	kernel/arch/ports.o \
	kernel/arch/tsc.o \
	kernel/arch/spinlock.o \
	kernel/dev/keyboard.o \

all: yak.elf

.PHONY: all clean install install-headers install-kernel

yak.elf: $(OBJS) kernel/boot/link.ld
	@echo "[LD]\t$@"
	@$(CC) -T kernel/boot/link.ld -o $@ $(CFLAGS) $(OBJS) $(LDFLAGS) $(LIBS)

	@$(OBJDUMP) -b elf64-x86-64 -S -d yak.elf > yak.txt
	@$(OBJCOPY) -I elf64-x86-64 --only-keep-debug yak.elf yal.sym
	@$(OBJCOPY) -I elf64-x86-64 --strip-debug yak.elf

kernel/boot/main.o: kernel/boot/main.c
	@if ! test -f build.num; then echo 0 > build.num; fi
	@echo $$(($$(cat build.num) + 1)) > build.num
	@echo "[CC]\t$@"
	@$(CC) -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

%.o: %.c
	@echo "[CC]\t$@"
	@$(CC) -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

%.o: %.S
	@echo "[CC]\t$@"
	@$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS) -D__ASSEMBLY__

kernel/boot/link.ld: kernel/boot/link.ld.S
	@echo "[CPP]\t$@"
	@$(CPP) -D__LINKER__ -Iinclude -P $< -o $@

clean:
	@rm -fv yak.elf $(OBJS) *.o */*.o */*/*.o kernel/boot/link.ld

#install: install-headers install-kernel

install-headers:
	@mkdir -p $(DESTDIR)$(INCLUDEDIR)
	@cp -RTv include $(DESTDIR)$(INCLUDEDIR)

install: yak.elf
	@cp -v yak.elf $(DESTDIR)
