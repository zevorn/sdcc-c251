        .module invalid_branch_range
        .area MCS251TEST (ABS,CODE)
        .org 0
        .binary

        sjmp far_target
        .org 0x0200
far_target:
        nop
