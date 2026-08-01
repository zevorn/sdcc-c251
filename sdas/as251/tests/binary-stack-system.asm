        .module binary_stack_system
        .area MCS251TEST (ABS,CODE)
        .org 0
        .binary

data = 0x30

        pop data
        pop acc
        pop a
        pop r5
        pop wr8
        pop dr20
        push data
        push acc
        push a
        push r5
        push wr8
        push dr20
        push #20
        push #0x55aa

        xch a,r5
        xch a,data
        xch a,@r1
        xch r5,r11
        xch data,a
        xch @r1,a
        xchd a,@r1
        xchd @r1,r11

        nop
        ret
        reti
        eret
        trap
        da a
        rl a
        rlc a
        rr a
        rrc a
        swap r11
        sll a
        sll r6
        sll wr12
        sra a
        sra r6
        sra wr12
        srl a
        srl r6
        srl wr12
