        .module source_smoke
        .area MCS251TEST (ABS,CODE)
        .org 0

        nop
        ret
        reti
        eret
        trap
        add a,r0
        add r3,r12
        mov wr4,wr10
        ecall 0x123456

        .nlist
        .area BIT_BANK (REL,OVR,DATA)
bits:
        .ds 1
        b0 = bits[0]
        b1 = bits[1]
        b7 = bits[7]

        .area MCS251BITCODE (CODE)
        mov bits,c
        mov c,bits

        .area BSEG (BIT)
bitvar:
        .ds 1
        .area MCS251BITCODE (CODE)
        mov bitvar,c
        mov c,bitvar
