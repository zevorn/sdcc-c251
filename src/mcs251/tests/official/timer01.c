/* Semantic source:
   https://www.stcaimcu.com/thread-24457-1-1.html

   The board-specific clock setup and long demonstration loop were replaced
   by bounded overflow vectors for QEMU's implemented Timer0/1 subset. */

#include "case-support.h"

static volatile unsigned char timer0_ticks;
static volatile unsigned char timer1_ticks;

static void
reload_timer0 (void)
{
    TH0 = 0xff;
    TL0 = 0xe0;
}

static void
reload_timer1 (void)
{
    TH1 = 0xff;
    TL1 = 0xe0;
}

void
timer0_interrupt (void) __interrupt (1)
{
    reload_timer0 ();
    timer0_ticks++;
}

void
timer1_interrupt (void) __interrupt (3)
{
    reload_timer1 ();
    timer1_ticks++;
}

void
main (void)
{
    unsigned long timeout;
    unsigned char passed = 1;

    TMOD = 0x11;
    AUXR = 0x00;

    reload_timer0 ();
    IE = IE_EA | IE_ET0;
    TCON = TCON_TR0;
    timeout = 200000UL;
    while (timer0_ticks < 3 && --timeout)
        {
        }
    TCON = 0;
    IE = 0;
    passed &= timer0_ticks >= 3;

    reload_timer1 ();
    IE = IE_EA | IE_ET1;
    TCON = TCON_TR1;
    timeout = 200000UL;
    while (timer1_ticks < 3 && --timeout)
        {
        }
    TCON = 0;
    IE = 0;
    passed &= timer1_ticks >= 3;

    case_report ("official-timer01", passed);
    for (;;)
        {
        }
}
