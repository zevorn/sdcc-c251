/* Semantic sources:
   https://www.stcaimcu.com/thread-24754-1-1.html
   https://www.keil.com/download/docs/46.asp

   This is an original bounded-vector test, not a translation of either
   vendor implementation. */

#include "case-support.h"

static unsigned char
check_byte (volatile unsigned char __xdata *address, unsigned char pattern)
{
    *address = pattern;
    return *address == pattern;
}

void
main (void)
{
    volatile unsigned char __xdata *boundary;
    unsigned char passed = 1;

    passed &= check_byte ((volatile unsigned char __xdata *)0x003fffUL,
                          0x3c);
    passed &= check_byte ((volatile unsigned char __xdata *)0x010000UL,
                          0x10);
    passed &= check_byte ((volatile unsigned char __xdata *)0x01ffffUL,
                          0x1f);
    passed &= check_byte ((volatile unsigned char __xdata *)0x020000UL,
                          0x20);
    passed &= check_byte ((volatile unsigned char __xdata *)0x02ffffUL,
                          0x2f);
    passed &= check_byte ((volatile unsigned char __xdata *)0x030000UL,
                          0x30);
    passed &= check_byte ((volatile unsigned char __xdata *)0x030fffUL,
                          0x3f);

    boundary = (volatile unsigned char __xdata *)0x01ffffUL;
    boundary[0] = 0xa5;
    boundary[1] = 0x5a;
    passed &= boundary[0] == 0xa5 && boundary[1] == 0x5a;

    boundary = (volatile unsigned char __xdata *)0x02ffffUL;
    boundary[0] = 0xc3;
    boundary[1] = 0x3c;
    passed &= boundary[0] == 0xc3 && boundary[1] == 0x3c;

    case_report ("official-sram", passed);
    for (;;)
        {
        }
}
