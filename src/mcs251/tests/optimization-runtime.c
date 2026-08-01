__sfr __at (0x99) SBUF;

static volatile unsigned int optimization_input;
static volatile signed char signed_char_input;
static volatile unsigned char unsigned_char_input;
static volatile unsigned int extension_addend;

static signed int
native_extend_signed_char (signed char value)
{
    return value;
}

static unsigned int
native_extend_unsigned_add (unsigned char value, unsigned int addend)
{
    return value + addend;
}

static unsigned int
native_and16_literal (unsigned int value)
{
    return value & 0x5aa5u;
}

static unsigned int
native_or16_literal (unsigned int value)
{
    return value | 0x5aa5u;
}

static unsigned int
native_xor16_literal (unsigned int value)
{
    return value ^ 0x5aa5u;
}

static unsigned char
native_less_than16_literal (unsigned int value)
{
    return value < 0x5aa5u;
}

static void
print_result (unsigned char passed)
{
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
}

void
main (void)
{
    unsigned char passed = 1;

    optimization_input = 0;
    if (native_and16_literal (optimization_input) != 0 ||
        native_or16_literal (optimization_input) != 0x5aa5u ||
        native_xor16_literal (optimization_input) != 0x5aa5u ||
        !native_less_than16_literal (optimization_input))
        passed = 0;

    optimization_input = 0x5aa4u;
    if (!native_less_than16_literal (optimization_input))
        passed = 0;

    optimization_input = 0x5aa5u;
    if (native_less_than16_literal (optimization_input))
        passed = 0;

    optimization_input = 0xffffu;
    if (native_and16_literal (optimization_input) != 0x5aa5u ||
        native_or16_literal (optimization_input) != 0xffffu ||
        native_xor16_literal (optimization_input) != 0xa55au ||
        native_less_than16_literal (optimization_input))
        passed = 0;

    signed_char_input = -128;
    if (native_extend_signed_char (signed_char_input) != -128)
        passed = 0;
    signed_char_input = -1;
    if (native_extend_signed_char (signed_char_input) != -1)
        passed = 0;
    signed_char_input = 0;
    if (native_extend_signed_char (signed_char_input) != 0)
        passed = 0;
    signed_char_input = 127;
    if (native_extend_signed_char (signed_char_input) != 127)
        passed = 0;

    extension_addend = 0x5a00u;
    unsigned_char_input = 0;
    if (native_extend_unsigned_add (unsigned_char_input,
                                    extension_addend) != 0x5a00u)
        passed = 0;
    unsigned_char_input = 127;
    if (native_extend_unsigned_add (unsigned_char_input,
                                    extension_addend) != 0x5a7fu)
        passed = 0;
    unsigned_char_input = 128;
    if (native_extend_unsigned_add (unsigned_char_input,
                                    extension_addend) != 0x5a80u)
        passed = 0;
    unsigned_char_input = 255;
    if (native_extend_unsigned_add (unsigned_char_input,
                                    extension_addend) != 0x5affu)
        passed = 0;

    print_result (passed);
    for (;;)
        {
        }
}
