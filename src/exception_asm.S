.section .text
.global vector_table
.extern exception_handler
.align 11

vector_table:
    b _exc_handler          // Synchronous Current EL1, SP0
    .align 7
    b _exc_handler          // IRQ Current EL1, SP0
    .align 7
    b _exc_handler          // FIQ Current EL1, SP0
    .align 7
    b _exc_handler          // SError Current EL1, SP0
    .align 7

    b _exc_handler          // Synchronous Current EL1, SPx
    .align 7
    b _exc_handler          // IRQ Current EL1, SPx
    .align 7
    b _exc_handler          // FIQ Current EL1, SPx
    .align 7
    b _exc_handler          // SError Current EL1, SPx
    .align 7

    b _exc_handler          // Synchronous Lower EL (EL0), AArch64
    .align 7
    b _exc_handler          // IRQ Lower EL (EL0), AArch64
    .align 7
    b _exc_handler          // FIQ Lower EL (EL0), AArch64
    .align 7
    b _exc_handler          // SError Lower EL (EL0), AArch64
    .align 7

    b _exc_handler          // Synchronous Lower EL (EL0), AArch32
    .align 7
    b _exc_handler          // IRQ Lower EL (EL0), AArch32
    .align 7
    b _exc_handler          // FIQ Lower EL (EL0), AArch32
    .align 7
    b _exc_handler          // SError Lower EL (EL0), AArch32
    .align 7

_exc_handler:
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

    mov x0, sp
    bl exception_handler

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
    eret