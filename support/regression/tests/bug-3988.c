/*
   bug-3988.c
   z80 codegen issue resulting in silent generation of wrong code
 */

#include <testfwk.h>

unsigned short FadeColor(unsigned short color, unsigned char step)
{
    signed short c1 = (color & 0x07) - step;
    signed short c2 = ((color & 0x070)>>4) - step;

    if (c1<0) c1=0;
    if (c2<0) c2=0;

    return c1 + (c2<<4);
}

void
testBug(void)
{
    ASSERT(FadeColor(0x077, 4) == 0x33);
}

