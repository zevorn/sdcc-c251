/* Semantic source:
   https://www.stcaimcu.com/forum-108-5.html (Timer0/1 external-counter topics)

   The host supplies exact falling-edge counts through the modeled P3.4/P3.5
   pins.  No wall-clock or board oscillator assumption is involved. */

#include "case-support.h"

static unsigned char
receive_command (void)
{
    unsigned char value;

    SCON |= SCON_REN;
    while (!(SCON & SCON_RI))
        {
        }
    value = SBUF;
    SCON &= (unsigned char)~SCON_RI;
    return value;
}

void
main (void)
{
    unsigned char passed;

    SCON = SCON_REN;
    P3M1 |= 0x30;
    P3M0 &= (unsigned char)~0x30;
    TMOD = 0x55;
    TH0 = 0;
    TL0 = 0;
    TH1 = 0;
    TL1 = 0;
    TCON = TCON_TR0 | TCON_TR1;
    case_serial_puts ("READY:official-counter01\n");
    passed = receive_command () == 'C';
    TCON = 0;
    passed &= TH0 == 0 && TL0 == 3;
    passed &= TH1 == 0 && TL1 == 5;
    case_report ("official-counter01", passed);
    for (;;)
        {
        }
}
