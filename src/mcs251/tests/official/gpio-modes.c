/* Semantic sources:
   https://www.stcmicro.com/cn/slcx.html (GPIO mode examples)
   https://www.stcaimcu.com/forum-108-5.html (STC32G144K246 GPIO topics)

   Each port uses two pins for each hardware mode.  Host-driven input
   patterns make the latch/input distinction observable without LEDs or
   board-specific wiring. */

#include "case-support.h"

#define CONFIGURE_PORT(PORT, MODE1, MODE0) \
    do                                         \
        {                                      \
            MODE1 = 0xf0;                      \
            MODE0 = 0xcc;                      \
            PORT = 0x75;                       \
        }                                      \
    while (0)

#define EXPECTED_PORT(INPUT) (((INPUT) & 0x71) | 0x04)

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
    CONFIGURE_PORT (P0, P0M1, P0M0);
    CONFIGURE_PORT (P1, P1M1, P1M0);
    CONFIGURE_PORT (P2, P2M1, P2M0);
    CONFIGURE_PORT (P3, P3M1, P3M0);
    CONFIGURE_PORT (P4, P4M1, P4M0);
    CONFIGURE_PORT (P5, P5M1, P5M0);
    CONFIGURE_PORT (P6, P6M1, P6M0);
    CONFIGURE_PORT (P7, P7M1, P7M0);

    case_serial_puts ("READY:official-gpio-modes\n");
    passed = receive_command () == 'M';
    passed &= P0 == EXPECTED_PORT (0x00);
    passed &= P1 == EXPECTED_PORT (0xff);
    passed &= P2 == EXPECTED_PORT (0x11);
    passed &= P3 == EXPECTED_PORT (0x22);
    passed &= P4 == EXPECTED_PORT (0x44);
    passed &= P5 == EXPECTED_PORT (0x55);
    passed &= P6 == EXPECTED_PORT (0xaa);
    passed &= P7 == EXPECTED_PORT (0xee);
    case_report ("official-gpio-modes", passed);
    for (;;)
        {
        }
}
