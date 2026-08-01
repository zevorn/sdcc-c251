        .module binary_inc_mul
        .area MCS251TEST (ABS,CODE)
        .org 0
        .binary

data = 0x30

        dec a
        dec data
        dec @r1
        dec r5,#1
        dec r10
        dec r11,#1
        dec a,#2
        dec r11,#4
        dec wr2,#2
        dec spx,#4
        inc a
        inc data
        inc @r1
        inc r5,#1
        inc r10
        inc r11,#1
        inc a,#2
        inc r11,#4
        inc wr2,#2
        inc spx,#4
        inc dptr
        mul ab
        mul r5,r7
        mul wr10,wr14
        div ab
        div r5,r7
        div wr10,wr14
