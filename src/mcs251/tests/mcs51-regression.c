__sfr __at (0x99) SBUF;
__xdata __at (0x0100) volatile unsigned char external_value;

static unsigned char
add_three (unsigned char value)
{
    return value + 3;
}

static unsigned char
checksum (unsigned char seed)
{
    unsigned char index;

    for (index = 0; index < 10; ++index)
        seed = (seed << 1) ^ index;

    return seed;
}

unsigned char
__sdcc_external_startup (void)
{
    return 0;
}

void
main (void)
{
    unsigned char __xdata *xdata_pointer = &external_value;
    unsigned char *generic_pointer = (unsigned char *)xdata_pointer;
    unsigned char (*function_pointer) (unsigned char) = add_three;
    volatile unsigned long dividend = 100000UL;
    volatile unsigned long quotient;
    volatile unsigned int word;
    unsigned char passed = 1;

    *xdata_pointer = 0x5a;
    if (*generic_pointer != 0x5a)
        passed = 0;

    if (function_pointer (4) != 7 || checksum (0x31) != 0x25)
        passed = 0;

    word = 0x1234U;
    word += 0x1111U;
    word <<= 1;
    if (word != 0x468aU)
        passed = 0;

    quotient = dividend / 10UL;
    if (quotient != 10000UL)
        passed = 0;

    if (passed)
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
