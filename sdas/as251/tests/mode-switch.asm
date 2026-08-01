        .module mode_switch
        .area MCS251TEST (ABS,CODE)
        .org 0

        .binary
        nop
        ret
        reti
        eret
        trap
        add a,r0
        add r3,r12
        mov wr4,wr10
        ecall 0x123456
        esc

        .source
        add a,r0
