# Usage : make [ARCH=x86|aarch64]   (aarch64 : CROSS=aarch64-linux-gnu- par defaut)
.DEFAULT_GOAL := all
.PHONY: linux-host test qemu iso clean
ARCH  ?= x86
CROSS ?= aarch64-linux-gnu-
COMMON = -ffreestanding -fno-builtin -nostdlib -fno-pic -fno-pie -fno-stack-protector \
         -fno-tree-loop-distribute-patterns -O2 -Wall -Wextra -DKERNEL -Iinclude
CORE   = core/kstring.c core/console.c core/heap.c core/ramfs.c core/shell.c core/kmain.c
OUT    = build/$(ARCH)
ELF    = talco-kernel-$(ARCH).elf

ifeq ($(ARCH),x86)
  CC = gcc -m32
  CFLAGS = $(COMMON)
  ASM = as --32
  LD = ld -m elf_i386
  BOOT = $(OUT)/boot.o
  HALSRC = arch/x86/hal_x86.c
$(OUT)/boot.o: arch/x86/boot.s
	@mkdir -p $(OUT) && as --32 $< -o $@
else
  CC = $(CROSS)gcc
  CFLAGS = $(COMMON) -mgeneral-regs-only -mstrict-align
  LD = $(CROSS)ld
  BOOT = $(OUT)/boot.o
  HALSRC = arch/aarch64/hal_aarch64.c
$(OUT)/boot.o: arch/aarch64/boot.S
	@mkdir -p $(OUT) && $(CC) $(CFLAGS) -c $< -o $@
endif

OBJS = $(BOOT) $(patsubst %.c,$(OUT)/%.o,$(CORE) $(HALSRC))

all: $(ELF)
$(OUT)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
$(ELF): $(OBJS) arch/$(ARCH)/linker.ld
	$(LD) -nostdlib -T arch/$(ARCH)/linker.ld -o $@ $(OBJS)

test:
	@mkdir -p build
	gcc -Wall -Wextra -Iinclude -o build/host_test core/kstring.c core/console.c core/heap.c core/ramfs.c core/shell.c tests/host_hal.c tests/host_test.c && ./build/host_test
qemu: $(ELF)
ifeq ($(ARCH),x86)
	qemu-system-i386 -kernel $(ELF) -serial stdio
else
	qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic -kernel $(ELF)
endif
iso: talco-kernel-x86.elf
	mkdir -p iso/boot && cp talco-kernel-x86.elf iso/boot/talco-kernel.elf && grub-mkrescue -o talco-os3.iso iso
clean:
	rm -rf build *.elf talco-os3.iso

linux-host:
	@mkdir -p build
	gcc -O2 -Wall -Wextra -Iinclude -DTALCO_LINUX -o build/talco-linux-host core/kstring.c core/console.c core/heap.c core/ramfs.c core/shell.c core/kmain.c arch/linux/hal_linux.c
