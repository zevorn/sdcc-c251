        .module binary_arithmetic_addressing
        .area MCS251TEST (ABS,CODE)
        .org 0
        .binary

data = 0x30

        add a,#0x12
        add a,data
        add a,0x1234
        add a,@r1
        add a,@wr2
        add a,@dr8
        add a,r4
        add a,a
        add a,r13
        add r3,r12
        add wr8,wr12
        add dr20,dpx
        add r4,#0x23
        add r11,#0x23
        add wr4,#0x1234
        add dr4,#0x1234
        add r8,data
        add r11,data
        add wr8,data
        add r8,0x1234
        add wr8,0x055a
        add r11,@r1
        add r2,@wr6
        add r6,@dr12

        sub a,r7
        sub wr4,wr10
        sub dr12,dr28
        sub r4,#10
        sub wr6,#1000
        sub dr16,#0x5aa5
        sub a,data
        sub wr8,data
        sub r12,0x1243
        sub wr10,0x1342
        sub r3,@wr14
        sub a,@spx

        cmp a,r7
        cmp wr4,wr10
        cmp dr12,dr28
        cmp r4,#10
        cmp wr6,#1000
        cmp dr16,#0x5aa5
        cmp dr16,#-3
        cmp a,data
        cmp wr8,data
        cmp r12,0x1243
        cmp wr10,0x1342
        cmp r3,@wr14
        cmp a,@spx

        addc a,r6
        addc r11,@r0
        addc a,data
        addc a,#0x20
        subb a,r6
        subb r11,@r0
        subb a,data
        subb a,#0x20
