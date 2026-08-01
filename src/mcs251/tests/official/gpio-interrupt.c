/* Semantic sources:
   https://www.stcmicro.com/cn/slcx.html (00/01 GPIO examples)
   https://www.stcaimcu.com/forum-108-5.html (INT0/1, gate, counter topics)

   This finite host-assisted protocol replaces LEDs, buttons, and board
   wiring with QEMU gpio-in/gpio-out observations. */

#include "case-support.h"

static volatile unsigned char int0_count;
static volatile unsigned char int1_count;

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

static void
observable_delay (void)
{
    volatile unsigned int count = 4000;

    while (--count)
        {
        }
}

void
int0_interrupt (void) __interrupt (0)
{
    int0_count++;
}

void
int1_interrupt (void) __interrupt (2)
{
    int1_count++;
}

void
main (void)
{
    unsigned long timeout;
    unsigned char passed = 1;
    unsigned char phase_passed;

    SCON = SCON_REN;

    /* Push-pull output: the host must observe both edges on P6.0/P6.1. */
    P6M1 = 0x00;
    P6M0 = 0xff;
    P6 = 0x00;
    case_serial_puts ("READY:official-gpio-output\n");
    passed &= receive_command () == 'G';
    P6 = 0x01;
    observable_delay ();
    P6 = 0x03;
    observable_delay ();
    P6 = 0x00;
    observable_delay ();
    case_serial_puts ("DONE:official-gpio-output\n");

    /* High-impedance input: the host drives P0.0 before acknowledging each
       phase, so the firmware observes the pin rather than its latch. */
    P0M1 |= 0x01;
    P0M0 &= (unsigned char)~0x01;
    P0 |= 0x01;
    case_serial_puts ("READY:official-gpio-input-high\n");
    passed &= receive_command () == 'H';
    passed &= (P0 & 0x01) != 0;
    case_serial_puts ("READY:official-gpio-input-low\n");
    passed &= receive_command () == 'L';
    phase_passed = (P0 & 0x01) == 0;
    passed &= phase_passed;
    case_report ("official-gpio-input", phase_passed);

    /* Edge-triggered INT0/INT1 on P3.2/P3.3. */
    P3M1 |= 0x0c;
    P3M0 &= (unsigned char)~0x0c;
    TCON = TCON_IT0 | TCON_IT1;
    IE = IE_EA | IE_EX0 | IE_EX1;
    case_serial_puts ("READY:official-gpio-interrupt\n");
    timeout = 200000UL;
    while ((int0_count < 1 || int1_count < 1) && --timeout)
        {
        }
    IE = 0;
    phase_passed = int0_count == 1 && int1_count == 1;
    passed &= phase_passed;
    case_report ("official-gpio-interrupt-edge", phase_passed);

    case_report ("official-gpio-interrupt", passed);
    for (;;)
        {
        }
}
