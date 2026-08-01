#include "case-support.h"

/* DMAIR starts a TFPU operation only for MOV direct,#immediate.  Keeping the
   command sequence in this SDAS251 block makes that architectural contract
   explicit while all result checking remains ordinary SDCC C. */
__data __at (0x30) volatile unsigned char tfpu_results[64];

void
tfpu_run_vectors (void) __naked
{
    __asm
        push    psw
        mov     psw,#0x08
        mov     _DMAIR,#0x31
        mov     _DMAIR,#0x32

        ; 1.5 + 2.25 = 3.75
        mov     dr4,#0x0000
        movh    dr4,#0x3fc0
        mov     dr0,#0x0000
        movh    dr0,#0x4010
        mov     _DMAIR,#0x1c
        mov     _tfpu_results,dr4

        ; 5.5 - 2.0 = 3.5
        mov     dr4,#0x0000
        movh    dr4,#0x40b0
        mov     dr0,#0x0000
        movh    dr0,#0x4000
        mov     _DMAIR,#0x1d
        mov     (_tfpu_results + 4),dr4

        ; -3.0 * 0.5 = -1.5
        mov     dr4,#0x0000
        movh    dr4,#0xc040
        mov     dr0,#0x0000
        movh    dr0,#0x3f00
        mov     _DMAIR,#0x1e
        mov     (_tfpu_results + 8),dr4

        ; 7.5 / 2.5 = 3.0
        mov     dr4,#0x0000
        movh    dr4,#0x40f0
        mov     dr0,#0x0000
        movh    dr0,#0x4020
        mov     _DMAIR,#0x1f
        mov     (_tfpu_results + 12),dr4

        ; sqrt(9.0) = 3.0
        mov     dr4,#0x0000
        movh    dr4,#0x4110
        mov     _DMAIR,#0x20
        mov     (_tfpu_results + 16),dr4

        ; Compare 1.0 < 2.0, then classify negative zero.
        mov     dr4,#0x0000
        movh    dr4,#0x3f80
        mov     dr0,#0x0000
        movh    dr0,#0x4000
        mov     _DMAIR,#0x21
        mov     a,r7
        mov     (_tfpu_results + 20),a
        mov     dr4,#0x0000
        movh    dr4,#0x8000
        mov     _DMAIR,#0x22
        mov     a,r7
        mov     (_tfpu_results + 21),a

        ; Floating-point to integer conversions.
        mov     dr4,#0x0000
        movh    dr4,#0x4228
        mov     _DMAIR,#0x23
        mov     a,r7
        mov     (_tfpu_results + 22),a
        mov     dr4,#0x0000
        movh    dr4,#0xc396
        mov     _DMAIR,#0x24
        mov     (_tfpu_results + 23),wr6
        mov     dr4,#0x0000
        movh    dr4,#0x4228
        mov     _DMAIR,#0x25
        mov     (_tfpu_results + 25),dr4

        ; Signed integer to floating-point conversions.
        mov     r7,#0xfb
        mov     _DMAIR,#0x27
        mov     (_tfpu_results + 29),dr4
        mov     wr6,#-300
        mov     _DMAIR,#0x28
        mov     (_tfpu_results + 33),dr4
        mov     dr4,#0xe240
        movh    dr4,#0x0001
        mov     _DMAIR,#0x29
        mov     (_tfpu_results + 37),dr4

        ; Exact zero-angle trigonometric identities.
        mov     dr4,#0x0000
        mov     _DMAIR,#0x2d
        mov     (_tfpu_results + 41),dr4
        mov     dr4,#0x0000
        mov     _DMAIR,#0x2e
        mov     (_tfpu_results + 45),dr4
        mov     dr4,#0x0000
        mov     _DMAIR,#0x2f
        mov     (_tfpu_results + 49),dr4
        mov     dr4,#0x0000
        mov     _DMAIR,#0x30
        mov     (_tfpu_results + 53),dr4

        ; Divide-by-zero exception, clear, and control readback.
        mov     dr4,#0x0000
        movh    dr4,#0x3f80
        mov     dr0,#0x0000
        mov     _DMAIR,#0x1f
        mov     (_tfpu_results + 57),dr4
        mov     _DMAIR,#0x33
        mov     a,r7
        mov     (_tfpu_results + 61),a
        mov     _DMAIR,#0x32
        mov     _DMAIR,#0x33
        mov     a,r7
        mov     (_tfpu_results + 62),a
        mov     r7,#0x02
        mov     _DMAIR,#0x36
        mov     r7,#0x00
        mov     _DMAIR,#0x35
        mov     a,r7
        mov     (_tfpu_results + 63),a

        ; Both clock-selection commands are architecturally accepted.
        mov     _DMAIR,#0x3f
        mov     _DMAIR,#0x3e

        pop     psw
        eret
    __endasm;
}

static unsigned char
check4 (unsigned char offset, unsigned char b0, unsigned char b1,
        unsigned char b2, unsigned char b3)
{
    return tfpu_results[offset] == b0 &&
           tfpu_results[offset + 1] == b1 &&
           tfpu_results[offset + 2] == b2 &&
           tfpu_results[offset + 3] == b3;
}

void
main (void)
{
    unsigned char passed;

    tfpu_run_vectors ();
    passed = check4 (0, 0x40, 0x70, 0x00, 0x00);
    passed &= check4 (4, 0x40, 0x60, 0x00, 0x00);
    passed &= check4 (8, 0xbf, 0xc0, 0x00, 0x00);
    passed &= check4 (12, 0x40, 0x40, 0x00, 0x00);
    passed &= check4 (16, 0x40, 0x40, 0x00, 0x00);
    passed &= tfpu_results[20] == 0x01;
    passed &= tfpu_results[21] == 0x0a;
    passed &= tfpu_results[22] == 42;
    passed &= tfpu_results[23] == 0xfe && tfpu_results[24] == 0xd4;
    passed &= check4 (25, 0x00, 0x00, 0x00, 0x2a);
    passed &= check4 (29, 0xc0, 0xa0, 0x00, 0x00);
    passed &= check4 (33, 0xc3, 0x96, 0x00, 0x00);
    passed &= check4 (37, 0x47, 0xf1, 0x20, 0x00);
    passed &= check4 (41, 0x00, 0x00, 0x00, 0x00);
    passed &= check4 (45, 0x3f, 0x80, 0x00, 0x00);
    passed &= check4 (49, 0x00, 0x00, 0x00, 0x00);
    passed &= check4 (53, 0x00, 0x00, 0x00, 0x00);
    passed &= check4 (57, 0x7f, 0x80, 0x00, 0x00);
    passed &= (tfpu_results[61] & 0x02) != 0;
    passed &= tfpu_results[62] == 0x00;
    passed &= tfpu_results[63] == 0x02;
    case_report ("official-tfpu", passed);
    for (;;)
        {
        }
}
