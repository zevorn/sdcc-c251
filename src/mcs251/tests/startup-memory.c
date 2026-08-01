__sfr __at (0x99) SBUF;

volatile __xdata unsigned char initialized_bytes[5] = {
    0x12, 0x34, 0x56, 0x78, 0x9a,
};
volatile __xdata unsigned char cleared_bytes[5];

unsigned char
__sdcc_external_startup (void)
{
    unsigned char i;

    /* QEMU resets RAM to zero.  Poison both ranges before the SDCC startup
       stages so this test proves that XINIT copies and XSEG clearing run. */
    for (i = 0; i < sizeof (initialized_bytes); ++i)
        {
            initialized_bytes[i] = 0xee;
            cleared_bytes[i] = 0xdd;
        }
    return 0;
}

static void
print_result (unsigned char passed)
{
    const char *text = passed ? "PASS\n" : "FAIL\n";

    while (*text)
        SBUF = *text++;
}

void
main (void)
{
    static const unsigned char expected[5] = {
        0x12, 0x34, 0x56, 0x78, 0x9a,
    };
    unsigned char passed = 1;
    unsigned char i;

    for (i = 0; i < sizeof (initialized_bytes); ++i)
        {
            passed &= initialized_bytes[i] == expected[i];
            passed &= cleared_bytes[i] == 0;
        }
    print_result (passed);
    for (;;)
        {
        }
}
