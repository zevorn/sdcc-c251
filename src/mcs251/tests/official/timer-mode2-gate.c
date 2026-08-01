/* Semantic sources:
   https://www.stcaimcu.com/thread-24457-1-1.html
   https://www.stcaimcu.com/forum-108-5.html (timer gate topics)

   QEMU drives P3.2/P3.3 as the physical gate inputs.  Mode 2 is observed
   through the hardware reload value; neither interrupt handler reloads TLx. */

#include "case-support.h"

static volatile unsigned char timer0_ticks;
static volatile unsigned char timer1_ticks;
static volatile unsigned char timer0_sample;
static volatile unsigned char timer1_sample;

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
    volatile unsigned long count = 30000UL;

    while (--count)
        {
        }
}

static void
report_timer_state (unsigned char timer, unsigned char ticks,
                    unsigned char sample, unsigned char high,
                    unsigned char low)
{
    case_serial_puts ("OBS:official-timer");
    case_serial_putc (timer);
    case_serial_putc (':');
    case_serial_puthex (ticks);
    case_serial_putc (':');
    case_serial_puthex (sample);
    case_serial_putc (':');
    case_serial_puthex (high);
    case_serial_putc (':');
    case_serial_puthex (low);
    case_serial_putc ('\n');
}

void
timer0_interrupt (void) __interrupt (1)
{
    timer0_sample = TL0;
    TCON &= (unsigned char)~TCON_TR0;
    timer0_ticks++;
}

void
timer1_interrupt (void) __interrupt (3)
{
    timer1_sample = TL1;
    TCON &= (unsigned char)~TCON_TR1;
    timer1_ticks++;
}

void
main (void)
{
    unsigned long timeout;
    unsigned char passed;
    unsigned char phase_passed;

    SCON = SCON_REN;
    P3M1 |= 0x0c;
    P3M0 &= (unsigned char)~0x0c;
    case_serial_puts ("READY:official-timer-mode2-gate-low\n");
    passed = receive_command () == 'L';

    TMOD = 0xaa;
    AUXR = 0x00;
    TH0 = 0x80;
    TL0 = 0xfe;
    TH1 = 0x80;
    TL1 = 0xfe;
    IE = IE_EA | IE_ET0 | IE_ET1;
    TCON = TCON_TR0 | TCON_TR1;
    observable_delay ();
    phase_passed = timer0_ticks == 0 && timer1_ticks == 0;
    phase_passed &= TL0 == 0xfe && TL1 == 0xfe;
    passed &= phase_passed;
    case_report ("official-timer-gate-low", phase_passed);

    case_serial_puts ("READY:official-timer-mode2-gate0\n");
    passed &= receive_command () == '0';
    timeout = 200000UL;
    while (timer0_ticks == 0 && --timeout)
        {
        }
    phase_passed = timer0_ticks == 1;
    phase_passed &= TH0 == 0x80 && timer0_sample >= 0x80;
    report_timer_state ('0', timer0_ticks, timer0_sample, TH0, TL0);
    passed &= phase_passed;
    case_report ("official-timer-mode2-0", phase_passed);

    case_serial_puts ("READY:official-timer-mode2-gate1\n");
    passed &= receive_command () == '1';
    timeout = 200000UL;
    while (timer1_ticks == 0 && --timeout)
        {
        }
    phase_passed = timer1_ticks == 1;
    phase_passed &= TH1 == 0x80 && timer1_sample >= 0x80;
    report_timer_state ('1', timer1_ticks, timer1_sample, TH1, TL1);
    passed &= phase_passed;
    case_report ("official-timer-mode2-1", phase_passed);

    IE = 0;
    TCON = 0;
    case_report ("official-timer-mode2-gate", passed);
    for (;;)
        {
        }
}
