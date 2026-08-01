        .module binary_branch_boundaries
        .area MCS251TEST (ABS,CODE)
        .binary

        .org 0x0000
        sjmp classic_forward
        .ds 0x7f
classic_forward:
        .ds 1
classic_backward:
        .ds 0x7e
        sjmp classic_backward

        .ds 0xfe
        je native_forward
        .ds 0x7f
native_forward:
        .ds 1
native_backward:
        .ds 0x7d
        je native_backward

        .ds 0xfd
        jb 0x30.5,bit_forward
        .ds 0x7f
bit_forward:
        .ds 1
bit_backward:
        .ds 0x7b
        jb 0x30.5,bit_backward

        .source
        .ds 0xfb
        cjne r6,#5,cjne_forward
        .ds 0x7f
cjne_forward:
        .ds 1
cjne_backward:
        .ds 0x7c
        cjne r6,#5,cjne_backward
