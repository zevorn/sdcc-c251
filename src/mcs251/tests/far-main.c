__sfr __at (0x99) SBUF;

unsigned char mcs251_far_add_one (unsigned char value);

unsigned char (*volatile far_operation) (unsigned char);

unsigned char
__sdcc_external_startup (void)
{
    return 0;
}

void
main (void)
{
    unsigned char value;

    far_operation = mcs251_far_add_one;
    value = far_operation (0x40);

    if (value == 0x41)
    {
        SBUF = 'P';
        SBUF = 'A';
        SBUF = 'S';
        SBUF = 'S';
    }
    else
    {
        SBUF = 'F';
        SBUF = 'A';
        SBUF = 'I';
        SBUF = 'L';
    }
    SBUF = '\n';

    for (;;)
    {
    }
}
