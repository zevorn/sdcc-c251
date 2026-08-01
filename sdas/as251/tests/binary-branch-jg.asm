        .module binary_branch_jg
        .area MCS251TEST (ABS,CODE)
        .org 0
        .binary

        jg target
target:
        nop
