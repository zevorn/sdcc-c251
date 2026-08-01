; Minimal MCS251 startup fragment for compiler and QEMU integration tests.
;
; ECALL/ERET use a three-byte return frame.  This file deliberately uses
; only the startup hooks emitted by SDCC so the test exercises the same
; initialization path as a normal linked program.

        .module mcs251_crt0
        .source

        .area CSEG    (CODE)
        .area GSINIT0 (CODE)
        .area GSINIT1 (CODE)
        .area GSINIT2 (CODE)
        .area GSINIT3 (CODE)
        .area GSINIT4 (CODE)
        .area GSINIT5 (CODE)
        .area GSINIT  (CODE)
        .area GSFINAL (CODE)

        .globl __start__stack

        .area GSINIT0 (CODE)

__sdcc_gsinit_startup::
        mov     spx,#__start__stack - 1

        .area GSINIT2 (CODE)

        ecall   ___sdcc_external_startup
        mov     a,dpl
        jz      __sdcc_init_data
        ejmp    __sdcc_program_startup
__sdcc_init_data:
