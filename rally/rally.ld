ENTRY(_start)

SECTIONS
{
    . = ALIGN(8);
    .text :
    {
        *(.entry)
        *(.text*)
        *(.rodata*)
    }

    . = ALIGN(8);
    .data : {
        *(.data*)
        *(.bss*)
    }

    . = ALIGN(8);
	.rela.dyn : {
        _reloc_start = .;
		*(.rela*)
        _reloc_end = .;
    }

    . = ALIGN(4096);
    .stack : {
        . = . + 0x2000;
        _stack_end = .;
    }
}