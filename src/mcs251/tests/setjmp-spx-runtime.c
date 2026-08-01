#include <setjmp.h>

__sfr __at (0x99) SBUF;
jmp_buf jump_buffer;

unsigned char
__sdcc_external_startup (void)
{
    return 0;
}

_Noreturn void
mcs251_longjmp_resume (void)
{
    longjmp (jump_buffer, 0x1234);
}

/* Exercise the library with a saved SPX above 0x00ff without using an
   indexed negative displacement in the test itself.  This isolates the
   setjmp ABI from QEMU's independently tested dis16 implementation. */
void
main (void) __naked
{
    __asm
        .globl ___setjmp
        .globl _mcs251_longjmp_resume

        mov     spx,#0x0120
        mov     dptr,#_jump_buffer
        mov     b,#(_jump_buffer >> 16)
        ecall   ___setjmp

        mov     a,dpl
        orl     a,dph
        jnz     mcs251_setjmp_resumed$

        ecall   _mcs251_longjmp_resume
        ejmp    mcs251_setjmp_failed$

mcs251_setjmp_resumed$:
        mov     a,dpl
        cjne    a,#0x34,mcs251_setjmp_failed$
        mov     a,dph
        cjne    a,#0x12,mcs251_setjmp_failed$
        mov     a,sp
        cjne    a,#0x20,mcs251_setjmp_failed$
        mov     a,sph
        cjne    a,#0x01,mcs251_setjmp_failed$

        mov     _SBUF,#0x50
        mov     _SBUF,#0x41
        mov     _SBUF,#0x53
        mov     _SBUF,#0x53
        mov     _SBUF,#0x0a
mcs251_setjmp_stopped$:
        ejmp    mcs251_setjmp_stopped$

mcs251_setjmp_failed$:
        mov     _SBUF,#0x46
        mov     _SBUF,#0x41
        mov     _SBUF,#0x49
        mov     _SBUF,#0x4c
        mov     _SBUF,#0x0a
        ejmp    mcs251_setjmp_stopped$
    __endasm;
}
