COMMON_MAKEFILE := $(lastword $(MAKEFILE_LIST))
COMMON_DIR := $(patsubst %/,%,$(dir $(COMMON_MAKEFILE)))

CFLAGS += -fshort-wchar -I$(COMMON_DIR) -I$(COMMON_DIR)/libc
OBJ += build/common/libc/stdio.o \
	build/common/libc/string.o \
	build/common/mach_o.o \
	build/common/pl011.o \
	build/common/serial.o \
	build/common/reloc.o \
	build/common/fb.o \

build/common/%.o: $(COMMON_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/common/%.o: $(COMMON_DIR)/%.S
	@mkdir -p $(dir $@)
	$(AS) $(CFLAGS) -c -o $@ $<