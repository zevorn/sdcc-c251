/* Semantic source:
   https://www.stcaimcu.com/thread-24465-1-1.html

   The PLL, pinmux, baud generator, and vendor ring buffer were intentionally
   removed.  QEMU models byte-level UART1 delivery, RI/TI, and its interrupt. */

#include "case-support.h"

static volatile unsigned char received[4];
static volatile unsigned char received_count;

void
uart1_interrupt (void) __interrupt (4)
{
    unsigned char value;

    if (SCON & SCON_RI)
        {
            value = SBUF;
            SCON &= (unsigned char)~SCON_RI;
            if (received_count < sizeof (received))
                received[received_count++] = value;
            SBUF = value;
        }
    if (SCON & SCON_TI)
        SCON &= (unsigned char)~SCON_TI;
}

void
main (void)
{
    unsigned long timeout;
    unsigned char passed;

    SCON = 0;
    case_serial_puts ("READY:official-uart1\n");
    SCON = SCON_REN;
    IE = IE_EA | IE_ES;

    timeout = 400000UL;
    while (received_count < sizeof (received) && --timeout)
        {
        }
    IE = 0;

    passed = received_count == sizeof (received);
    passed &= received[0] == 0x00;
    passed &= received[1] == 0x7f;
    passed &= received[2] == 0x80;
    passed &= received[3] == 0xff;
    case_report ("official-uart1", passed);
    for (;;)
        {
        }
}
