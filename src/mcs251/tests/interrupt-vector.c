volatile unsigned char timer_ticks;

void
timer0_interrupt (void) __interrupt (1)
{
    timer_ticks++;
}

void
main (void)
{
    for (;;)
        {
        }
}
