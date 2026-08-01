        .module binary_control_flow
        .area MCS251TEST (ABS,CODE)
        .org 0xff0000
        .binary

data = 0x30

        acall target
        ajmp target
        lcall target
        lcall @wr4
        ljmp target
        ljmp @wr2
        ecall target
        ecall @dpx
        ejmp target
        ejmp @dr16
        sjmp target
        jc target
        jnc target
        jnz target
        jz target
        je target
        jsle target
        jle target
        jne target
        jsg target
        jsge target
        jsl target
        jsle target

        jmp @a+dptr
        jmp @wr2
        jmp @dr16
        jmp target
        jmp target+0x4000
        jmp 0x4000
        call @wr4
        call @dpx
        call target
        call target+0x4000

        jb 0x0f,target
        jbc 0x94,target
        jnb 0xe3,target
        jb 0x30.5,target
        jnb s:0x91.4,target
        jbc 0x44.5,target

        djnz r5,target
        djnz acc,target
        djnz data,target
        cjne a,data,target
        cjne a,#'A',target
        cjne r6,#50,target
        cjne @r1,#0x40,target

target:
