#include <setjmp.h>

__sfr __at (0x99) SBUF;
jmp_buf jump_buffer;
static jmp_buf recursive_jump_buffer;
static jmp_buf zero_jump_buffer;
static jmp_buf live_register_jump_buffer;
static volatile unsigned char recursive_call_count;
static volatile unsigned int live_register_seed = 0x1234u;
static volatile unsigned int live_register_other = 0xabcdu;

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

static int
recursive_setjmp (unsigned char depth) __reentrant
{
    volatile unsigned char padding[48];
    int resumed;

    ++recursive_call_count;
    padding[0] = depth;
    padding[47] = depth + 1;
    if (depth)
        return recursive_setjmp (depth - 1) + padding[0] - depth;

    resumed = setjmp (recursive_jump_buffer);
    if (!resumed)
        longjmp (recursive_jump_buffer, 0x1234);
    return resumed + padding[47] - 1;
}

static unsigned char
recursive_setjmp_is_big_endian (void)
{
    int resumed = setjmp (zero_jump_buffer);

    if (!resumed)
        longjmp (zero_jump_buffer, 0);
    recursive_call_count = 0;
    return resumed == 1 && recursive_setjmp (6) == 0x1234 &&
           recursive_call_count == 7;
}

static unsigned char
live_registers_survive_longjmp (void)
{
    unsigned int first = live_register_seed;
    unsigned int second = live_register_other;
    int resumed = setjmp (live_register_jump_buffer);

    if (!resumed)
        longjmp (live_register_jump_buffer, 0);
    return resumed == 1 && first == 0x1234u && second == 0xabcdu;
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

mcs251_setjmp_return$:
        mov     a,dpl
        orl     a,dph
        jnz     mcs251_setjmp_resumed$

        ; jmp_buf stores SPX and the return PC in native byte order.
        mov     dptr,#_jump_buffer
        mov     dpxl,#(_jump_buffer >> 16)
        mov     a,@dpx
        cjne    a,#0x01,mcs251_setjmp_failed$
        inc     dpx
        mov     a,@dpx
        cjne    a,#0x23,mcs251_setjmp_failed$
        inc     dpx
        mov     a,@dpx
        cjne    a,#(mcs251_setjmp_return$ >> 16),mcs251_setjmp_failed$
        inc     dpx
        mov     a,@dpx
        cjne    a,#(mcs251_setjmp_return$ >> 8),mcs251_setjmp_failed$
        inc     dpx
        mov     a,@dpx
        cjne    a,#mcs251_setjmp_return$,mcs251_setjmp_failed$

        ecall   _mcs251_longjmp_resume
        ejmp    mcs251_setjmp_failed$

mcs251_setjmp_resumed$:
        mov     a,dpl
        cjne    a,#0x34,mcs251_setjmp_failed$
        mov     a,dph
        cjne    a,#0x12,mcs251_setjmp_failed$
        mov     dptr,#(_jump_buffer + 5)
        mov     dpxl,#((_jump_buffer + 5) >> 16)
        mov     a,@dpx
        cjne    a,#0x12,mcs251_setjmp_failed$
        inc     dpx
        mov     a,@dpx
        cjne    a,#0x34,mcs251_setjmp_failed$
        mov     a,sp
        cjne    a,#0x20,mcs251_setjmp_failed$
        mov     a,sph
        cjne    a,#0x01,mcs251_setjmp_failed$
        ecall   _recursive_setjmp_is_big_endian
        mov     a,dpl
        jz      mcs251_setjmp_failed$
        ecall   _live_registers_survive_longjmp
        mov     a,dpl
        jz      mcs251_setjmp_failed$

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
