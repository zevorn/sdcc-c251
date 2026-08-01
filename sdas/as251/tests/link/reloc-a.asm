        .module reloc_a
        .globl target
        .area MCS251CODE (REL,CON,CODE)

start::
        ecall target
        ejmp target
        .3byte target
        .byte target, (target >> 8), (target >> 16)
        lcall target
        ljmp target
        acall target
        ajmp target
