volatile unsigned char mcs251_debug_marker;

static void
mcs251_debug_step (unsigned char value)
{
    mcs251_debug_marker = value;
}

void
main (void)
{
    mcs251_debug_marker = 0x25;
    mcs251_debug_step (0x51);

    for (;;)
        ;
}
