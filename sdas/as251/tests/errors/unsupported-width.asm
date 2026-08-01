        .module unsupported_width
        .area MCS251TEST (ABS,CODE)
        .org 0
        .source

        anl dr4,dr8
        mov dr12,@dr4
        mul dr4,dr8
