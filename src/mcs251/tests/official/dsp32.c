#include "case-support.h"

/*
 * DSP32 operands live in architectural registers, so a naked helper uses
 * register bank 1 while the surrounding SDCC program keeps its normal bank.
 * Results are copied to fixed DATA RAM and checked by ordinary SDCC C code.
 */
__data __at (0x30) volatile unsigned char dsp_results[64];

void
dsp_run_vectors (void) __naked
{
    __asm
        push    psw
        mov     psw,#0x08

        ; Select fixed DATA bytes 0..7 as the DSP C/D register pair.
        mov     a,#0x00
        mov     _DPUOP,#0x80

        ; Binary-to-BCD and its inverse.
        mov     dr0,#0xcd15
        movh    dr0,#0x075b
        mov     _DPUOP,#0x81
        mov     _dsp_results,dr4
        mov     (_dsp_results + 4),dr0
        mov     _DPUOP,#0x82
        mov     (_dsp_results + 8),dr4

        ; Normalization and the reported shift count.
        mov     dr0,#0x0000
        movh    dr0,#0x0100
        mov     _DPUOP,#0x84
        mov     (_dsp_results + 12),dr0
        mov     a,_DPUST
        mov     (_dsp_results + 16),a

        ; Register exchange.
        mov     dr4,#0x0001
        mov     dr0,#0x0002
        mov     _DPUOP,#0x86
        mov     (_dsp_results + 17),dr4
        mov     (_dsp_results + 21),dr0

        ; 32-bit arithmetic and flags.
        mov     dr4,#-1
        movh    dr4,#0x7fff
        mov     dr0,#0x0001
        mov     _DPUOP,#0x92
        mov     (_dsp_results + 25),dr4
        mov     a,psw
        mov     (_dsp_results + 29),a

        ; Unsigned 16-bit multiply and unsigned 32-bit divide.
        mov     dr4,#0xffff
        mov     dr0,#0x0002
        mov     _DPUOP,#0x9c
        mov     (_dsp_results + 30),dr4
        mov     dr4,#0x0011
        mov     dr0,#0x0005
        mov     _DPUOP,#0x9f
        mov     (_dsp_results + 34),dr4
        mov     (_dsp_results + 38),dr0

        ; Divide-by-zero status is observable and leaves the operands intact.
        mov     dr4,#0x0011
        mov     dr0,#0x0000
        mov     _DPUOP,#0x9f
        mov     a,_DPUST
        mov     (_dsp_results + 42),a

        ; Unary, accumulate, logical, and shift categories.
        mov     dr4,#0x0005
        mov     _DPUOP,#0xb8
        mov     (_dsp_results + 43),dr4
        mov     _DPUOP,#0xc0
        mov     (_dsp_results + 47),dr4
        mov     dr4,#0x00ff
        movh    dr4,#0x0f0f
        mov     dr0,#0xffff
        movh    dr0,#0x3333
        mov     _DPUOP,#0xd4
        mov     (_dsp_results + 51),dr0
        mov     dr4,#0x0001
        movh    dr4,#0x8000
        mov     a,#0x01
        mov     _DPUOP,#0xe4
        mov     (_dsp_results + 55),dr4

        ; Multiply-accumulate: EDX = EDX + EAX * EBX.
        mov     dr0,#0x0005
        mov     0x00,dr0
        mov     dr4,#0x0003
        mov     dr0,#0x0004
        mov     _DPUOP,#0xf4
        mov     a,0x00
        mov     (_dsp_results + 59),a
        mov     a,0x01
        mov     (_dsp_results + 60),a
        mov     a,0x02
        mov     (_dsp_results + 61),a
        mov     a,0x03
        mov     (_dsp_results + 62),a

        pop     psw
        eret
    __endasm;
}

static unsigned char
check4 (unsigned char offset, unsigned char b0, unsigned char b1,
        unsigned char b2, unsigned char b3)
{
    return dsp_results[offset] == b0 &&
           dsp_results[offset + 1] == b1 &&
           dsp_results[offset + 2] == b2 &&
           dsp_results[offset + 3] == b3;
}

void
main (void)
{
    unsigned char passed;

    dsp_run_vectors ();
    passed = check4 (0, 0x00, 0x00, 0x00, 0x01);
    passed &= check4 (4, 0x23, 0x45, 0x67, 0x89);
    passed &= check4 (8, 0x07, 0x5b, 0xcd, 0x15);
    passed &= check4 (12, 0x40, 0x00, 0x00, 0x00);
    passed &= (dsp_results[16] & 0x3f) == 6;
    passed &= check4 (17, 0x00, 0x00, 0x00, 0x02);
    passed &= check4 (21, 0x00, 0x00, 0x00, 0x01);
    passed &= check4 (25, 0x80, 0x00, 0x00, 0x00);
    passed &= (dsp_results[29] & 0x84) == 0x04;
    passed &= check4 (30, 0x00, 0x01, 0xff, 0xfe);
    passed &= check4 (34, 0x00, 0x00, 0x00, 0x03);
    passed &= check4 (38, 0x00, 0x00, 0x00, 0x02);
    passed &= (dsp_results[42] & 0x40) != 0;
    passed &= check4 (43, 0xff, 0xff, 0xff, 0xfb);
    passed &= check4 (47, 0xff, 0xff, 0xff, 0xfc);
    passed &= check4 (51, 0x3c, 0x3c, 0xff, 0x00);
    passed &= check4 (55, 0x40, 0x00, 0x00, 0x00);
    passed &= check4 (59, 0x00, 0x00, 0x00, 0x11);
    case_report ("official-dsp32", passed);
    for (;;)
        {
        }
}
