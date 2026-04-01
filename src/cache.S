.global cache_enable, cache_disable
cache_enable:
    ic iallu
    dsb sy
    isb

    mrs x0, sctlr_el1
    mov x1, #0x1004
    orr x0, x0, x1
    msr sctlr_el1, x0
    dsb sy
    isb

    ret

cache_disable:
    mrs x0, sctlr_el1
    mov x1, #~0x1004
    and x0, x0, x1
    msr sctlr_el1, x0
    dsb sy
    isb

    ic iallu
    dsb sy
    isb

    ret