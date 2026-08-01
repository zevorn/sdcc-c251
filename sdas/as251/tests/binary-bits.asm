        .module binary_bits
        .area MCS251TEST (ABS,CODE)
        .org 0
        .binary

        anl c,0x0f
        anl cy,0x30.5
        anl c,/0x0f
        anl cy,/0x30.5
        orl c,0x0f
        orl cy,0x30.5
        orl c,/0x0f
        orl cy,/0x30.5

        mov c,0x0f
        mov cy,0x30.5
        mov 0x0f,c
        mov s:0x92.6,cy

        clr a
        clr c
        clr 0x0f
        clr s:0x92.6
        cpl a
        cpl c
        cpl 0x0f
        cpl s:0x92.6
        setb c
        setb 0x0f
        setb s:0x92.6
