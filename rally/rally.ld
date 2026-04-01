ENTRY(_start)

SECTIONS
{
    PROVIDE (el1_sp0_synchronous_vector_long=0xfffffff007d23000);
    PROVIDE (el1_sp1_synchronous_vector_long=0xfffffff007d23594);
    PROVIDE (arm_vm_init=0xfffffff007e85608);
    PROVIDE (kernel_debug_early=0xfffffff00812955c);
    PROVIDE (sleh_irq=0xfffffff007e8a8d8);
    PROVIDE (arm_vm_prot_init=0xfffffff007e84430);
    PROVIDE (serial_init=0xfffffff00840fc1c);
    PROVIDE (uart_initted=0xfffffff00a3ae1c8);
    PROVIDE (serial_do_transmit=0xfffffff00a31fad8);
    PROVIDE (serial_irq_status=0xfffffff00a3ae210);
    PROVIDE (gPESF=0xfffffff00a3ae1c0);
    PROVIDE (DTInitialized=0xfffffff00793cd88);
    PROVIDE (startingP=0xfffffff00a3ae110);
    PROVIDE (DTRootNode=0xfffffff00793cd70);
    PROVIDE (ExceptionVectorsBase=0xfffffff007d22000);
    PROVIDE (LowExceptionVectorBase=0xfffffff007d29000);
    PROVIDE (mh_execute_header=0xfffffff007004000);

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