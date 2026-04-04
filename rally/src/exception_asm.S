.section .text
.global _exception_handler_sp0, _exception_handler_sp1, _low_exception_handler
.extern exception_handler, el1_sp0_synchronous_vector_long, el1_sp1_synchronous_vector_long

.macro SAVE_REGS
    mov x17, sp
    adrp x16, _stack_end
    add  x16, x16, #:lo12:_stack_end
    mov  sp, x16

    sub sp, sp, #0x100

    stp x0,  x1,  [sp, #0x000]
    stp x2,  x3,  [sp, #0x010]
    stp x4,  x5,  [sp, #0x020]
    stp x6,  x7,  [sp, #0x030]
    stp x8,  x9,  [sp, #0x040]
    stp x10, x11, [sp, #0x050]
    stp x12, x13, [sp, #0x060]
    stp x14, x15, [sp, #0x070]
    stp x16, x17, [sp, #0x080]
    stp x18, x19, [sp, #0x090]
    stp x20, x21, [sp, #0x0A0]
    stp x22, x23, [sp, #0x0B0]
    stp x24, x25, [sp, #0x0C0]
    stp x26, x27, [sp, #0x0D0]
    stp x28, x29, [sp, #0x0E0]
    str x30,      [sp, #0x0F0]
.endm

.macro RESTORE_REGS
    ldr x30,      [sp, #0x0F0]
    ldp x28, x29, [sp, #0x0E0]
    ldp x26, x27, [sp, #0x0D0]
    ldp x24, x25, [sp, #0x0C0]
    ldp x22, x23, [sp, #0x0B0]
    ldp x20, x21, [sp, #0x0A0]
    ldp x18, x19, [sp, #0x090]
    ldp x16, x17, [sp, #0x080]
    ldp x14, x15, [sp, #0x070]
    ldp x12, x13, [sp, #0x060]
    ldp x10, x11, [sp, #0x050]
    ldp x8,  x9,  [sp, #0x040]
    ldp x6,  x7,  [sp, #0x030]
    ldp x4,  x5,  [sp, #0x020]
    ldp x2,  x3,  [sp, #0x010]
    ldp x0,  x1,  [sp, #0x000]

    add sp, sp, #0x100

    mov sp, x17
.endm

_exception_handler_sp0:
    SAVE_REGS

    mov x0, sp
    bl exception_handler
    cbz w0, restore_chain_sp0

restore_eret_sp0:
    RESTORE_REGS
    eret

restore_chain_sp0:
    RESTORE_REGS

    adrp x16, symbols
    add  x16, x16, :lo12:symbols
    // It's bad that the offset is hardcoded here but I don't know easy way to fix this
    ldr  x16, [x16, #136] // symbols.exception.el1_sp0_synchronous_vector_long
    br x16

_exception_handler_sp1:
    SAVE_REGS

    mov x0, sp
    bl exception_handler
    cbz w0, restore_chain_sp1

restore_eret_sp1:
    RESTORE_REGS
    eret

restore_chain_sp1:
    RESTORE_REGS

    adrp x16, symbols
    add  x16, x16, :lo12:symbols
    // It's bad that the offset is hardcoded here but I don't know easy way to fix this
    ldr  x16, [x16, #144] // symbols.exception.el1_sp1_synchronous_vector_long
    br   x16

_low_exception_handler:
    SAVE_REGS

    mov x0, sp
    bl exception_handler
    cbz w0, .

    RESTORE_REGS
    eret