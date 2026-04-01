.section .text
.global _head
.extern main
.extern vector_table

_head:
	mov x0, x0
	b _start

_header:
    .quad 0           /* Text Offset */
    .quad 0           /* Image Size */
    .quad 0           /* Flags */
    .quad 0           /* Reserved */
    .quad 0           /* Reserved */
    .quad 0           /* Reserved */
    .ascii "ARM\x64"  /* ARM64 Magic */
    .long 0           /* Reserved */

_start:
    adrp x0, _stack_end
    add  x0, x0, #:lo12:_stack_end
    mov sp, x0

    adrp x0, _bss_start
    add  x0, x0, #:lo12:_bss_start
    adrp x1, _bss_end
    add  x1, x1, #:lo12:_bss_end
1:
    cmp x0, x1
    b.hs 2f
    stp xzr, xzr, [x0], #16
    b 1b
2:
    adrp x0, vector_table
    add  x0, x0, #:lo12:vector_table
    msr vbar_el1, x0

    adrp x0, _head
    add  x0, x0, #:lo12:_head

    adrp x1, _payload_start
    add  x1, x1, #:lo12:_payload_start

    adrp x2, _payload_end
    add  x2, x2, #:lo12:_payload_end

    sub x2, x2, x1

    bl main
    b .