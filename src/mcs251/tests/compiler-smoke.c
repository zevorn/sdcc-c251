#include <setjmp.h>

volatile unsigned char counter;
__sfr __at (0x99) SBUF;

_Static_assert (sizeof (void *) == 3, "MCS251 generic pointers must be 24-bit");
_Static_assert (sizeof (unsigned char __xdata *) == 3,
                "MCS251 far data pointers must be 24-bit");
_Static_assert (sizeof (unsigned char __code *) == 3,
                "MCS251 code pointers must be 24-bit");
_Static_assert (sizeof (jmp_buf) == 15,
                "MCS251 jmp_buf must hold SPX, PC, value, and R0-R7");

unsigned char
__sdcc_external_startup (void)
{
    return 0;
}

unsigned char
add_one (unsigned char value)
{
    return value + 1;
}

void
main (void)
{
    counter = add_one (counter);
    if (counter == 1)
    {
        SBUF = 'P';
        SBUF = 'A';
        SBUF = 'S';
        SBUF = 'S';
    }
    else
    {
        SBUF = 'F';
        SBUF = 'A';
        SBUF = 'I';
        SBUF = 'L';
    }
    SBUF = '\n';
}
