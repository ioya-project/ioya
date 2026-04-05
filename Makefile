CROSS_COMPILE = aarch64-linux-gnu-

CC = clang --target=$(CROSS_COMPILE)
AS = clang --target=$(CROSS_COMPILE)
LD = ld.lld
OBJCOPY = llvm-objcopy

TEXT_BASE ?= 0x80000000

IOYA_VERSION = 1.0.0-$(shell git log --pretty=format:"%h" | head -1)

CFLAGS = -O1 -Wall -ffreestanding -nostdinc -fpie -fpic -mstrict-align -fno-stack-protector -mgeneral-regs-only -Isrc -DTEXT_BASE=$(TEXT_BASE) -DIOYA_VERSION='"$(IOYA_VERSION)"' -g
LDFLAGS = -T ioya.ld -Ttext=$(TEXT_BASE) -nostdlib -static --format=default -pie
OBJ := build/start.o \
	build/main.o \
	build/exception.o \
	build/exception_asm.o \
	build/heap.o \
	build/dlmalloc/malloc.o \
	build/injector.o \
	build/libadt.o \
	build/nvram.o \
	build/block_dev.o \
	build/virtio_blk.o \
	build/ufshc_blk.o \
	build/gpt.o \
	build/fat.o \
	build/config_parser.o \
	build/cache.o

include common/Makefile

QEMU ?= 0

ifeq ($(QEMU), 1)
CFLAGS += -DQEMU=1
endif

all: build/ioya.bin

rally:
	$(MAKE) -C rally

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: src/%.S
	@mkdir -p $(dir $@)
	$(AS) $(CFLAGS) -c -o $@ $<

build/ioya.elf: $(OBJ) | rally
	$(LD) $(LDFLAGS) -o $@ $^

build/ioya.bin: build/ioya.elf
	$(OBJCOPY) -O binary $< $@