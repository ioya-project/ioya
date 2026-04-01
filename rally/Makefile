ifndef XNU_SOURCES
$(error XNU_SOURCES not set!)
endif

CROSS_COMPILE = aarch64-linux-gnu-

CC = clang --target=$(CROSS_COMPILE)
AS = clang --target=$(CROSS_COMPILE)
LD = ld.lld
OBJCOPY = llvm-objcopy

TEXT_BASE ?= 0x0

INCLUDES = -isystem$(XNU_SOURCES)/bsd \
	-isystem$(XNU_SOURCES)/osfmk \
	-isystem$(XNU_SOURCES)/osfmk/libsa \
	-isystem$(XNU_SOURCES)/libkern \
	-isystem$(XNU_SOURCES)/libsyscall/mach \
	-isystem$(XNU_SOURCES)/iokit \
	-isystem$(XNU_SOURCES)/pexpert \
	-isystem$(XNU_SOURCES)/EXTERNAL_HEADERS
DEFINES = -DRALLY -DTEXT_BASE=$(TEXT_BASE) -DKERNEL -DPRIVATE -DKERNEL_PRIVATE -DXNU_KERNEL_PRIVATE -DPEXPERT_KERNEL_PRIVATE -DVMAPPLE -D__arm64__ -D__LITTLE_ENDIAN__
CFLAGS = -O3 -Wall -ffreestanding -nostdinc -fpie -fpic -mstrict-align -fno-stack-protector -mgeneral-regs-only -fblocks -g $(DEFINES) $(INCLUDES) -Isrc
LDFLAGS = -T rally.ld -Ttext=$(TEXT_BASE) -nostdlib -static -pie

OBJ := build/main.o \
	build/hook.o \
	build/serial.o \
	build/device_tree.o \
	build/patcher.o \
	build/kernel_patches.o \
	build/kernel_hooks.o \
	build/exception_asm.o \
	build/exception.o

include ../common/Makefile

QEMU ?= 0

ifeq ($(QEMU), 1)
CFLAGS += -DQEMU=1
endif

all: build/rally.bin

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: src/%.S
	@mkdir -p $(dir $@)
	$(AS) $(CFLAGS) -c -o $@ $<

build/rally.elf: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

build/rally.bin: build/rally.elf
	$(OBJCOPY) -O binary $< $@