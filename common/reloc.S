.global apply_relocations

apply_relocations:
    stp x29, x30, [sp, #-32]!

    ldr x1, =TEXT_BASE
    sub x0, x0, x1

    adr x2, _reloc_start
    adr x3, _reloc_end
1:
    cmp x2, x3
    b.hi 2f
    ldp x1, x4, [x2], #16
    ldr x5, [x2], #8
    and x4, x4, #0xffffffff
    cmp x4, #1027
    b.ne 1b

    add x5, x5, x0
    add x1, x1, x0
    str x5, [x1]
    b 1b
2:
    ldp x29, x30, [sp], #32
    ret