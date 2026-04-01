ENTRY(_head)
TARGET(binary)
INPUT(rally/build/rally.bin)

SECTIONS
{
    . = ALIGN(8);
    .text : {
        *(.text*)
    }

    . = ALIGN(4096);
    .payload : {
        _payload_start = .;
        rally/build/rally.bin
        _payload_end = .;
    }

    . = ALIGN(8);
    .rodata : {
        *(.rodata*)
    }

    . = ALIGN(8);
    .data : {
        *(.data*)
    }

    . = ALIGN(8);
	.rela.dyn : {
        _reloc_start = .;
		*(.rela*)
        _reloc_end = .;
    }

    . = ALIGN(8);
    .bss : {
        _bss_start = .;
        *(.bss*)
        _bss_end = .;
    }

    . = ALIGN(4096);
    .stack (NOLOAD) : {
        . = . + 0x8000;
        _stack_end = .;
    }

    . = ALIGN(4096);
    .heap (NOLOAD) : {
        _heap_start = .;
        . = . + 0x20000000;
        _heap_end = .;
    }

	/DISCARD/ : { *(.dynsym) }
	/DISCARD/ : { *(.dynstr*) }
	/DISCARD/ : { *(.dynamic*) }
	/DISCARD/ : { *(.gnu*) }
	/DISCARD/ : { *(.hash) }
}