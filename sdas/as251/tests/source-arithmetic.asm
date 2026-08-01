        .module source_arithmetic
        .area MCS251TEST (ABS,CODE)
        .org 0

        add r3,r12
        sub wr4,wr10
        cmp dr12,dr28
        inc r10
        dec wr2,#2
        mul r5,r7
        div wr10,wr14
        sra r6
        srl wr12
        sll a
        addc a,r6
