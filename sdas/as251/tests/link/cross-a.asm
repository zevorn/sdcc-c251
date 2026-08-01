        .module cross_a
        .globl cross_target
        .area CALLER (REL,CON,CODE)

        lcall cross_target
        ljmp cross_target
