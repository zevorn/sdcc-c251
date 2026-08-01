__sfr __at (0x99) SBUF;

static volatile unsigned int optimization_input;
static volatile signed char signed_char_input;
static volatile unsigned char unsigned_char_input;
static volatile unsigned int extension_addend;
static volatile unsigned long dword_input;
static volatile unsigned long left_dword_input;
static volatile unsigned long right_dword_input;
static volatile unsigned int left_word_input;
static volatile unsigned int right_word_input;

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

static unsigned long
native_replace_low_word (unsigned long value)
{
    return (value & 0xffff0000ul) | 0x5678ul;
}

static unsigned long
native_replace_high_word (unsigned long value)
{
    return (value & 0x0000fffful) | 0x56780000ul;
}

static unsigned long
native_add_register_longs (const volatile unsigned long *left,
                           const volatile unsigned long *right)
{
    unsigned long left_value = *left;
    unsigned long right_value = *right;

    left_value += right_value;
    return left_value ^ right_value;
}

static unsigned long
native_subtract_register_longs (const volatile unsigned long *left,
                                const volatile unsigned long *right)
{
    unsigned long left_value = *left;
    unsigned long right_value = *right;

    left_value -= right_value;
    return left_value ^ right_value;
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

static unsigned int
native_and16_words (unsigned int left, unsigned int right)
{
    return left & right;
}

static unsigned int
native_or16_words (unsigned int left, unsigned int right)
{
    return left | right;
}

static unsigned int
native_xor16_words (unsigned int left, unsigned int right)
{
    return left ^ right;
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

    left_word_input = 0x12e4u;
    right_word_input = 0xa50fu;
    if (native_and16_words (left_word_input, right_word_input) != 0x0004u ||
        native_or16_words (left_word_input, right_word_input) != 0xb7efu ||
        native_xor16_words (left_word_input, right_word_input) != 0xb7ebu)
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

    dword_input = 0x00000000ul;
    if (native_replace_low_word (dword_input) != 0x00005678ul)
        passed = 0;
    dword_input = 0x1234abcdul;
    if (native_replace_low_word (dword_input) != 0x12345678ul)
        passed = 0;
    dword_input = 0xffff0000ul;
    if (native_replace_low_word (dword_input) != 0xffff5678ul)
        passed = 0;
    dword_input = 0x89abcdeful;
    if (native_replace_low_word (dword_input) != 0x89ab5678ul)
        passed = 0;

    dword_input = 0x00000000ul;
    if (native_replace_high_word (dword_input) != 0x56780000ul)
        passed = 0;
    dword_input = 0x1234abcdul;
    if (native_replace_high_word (dword_input) != 0x5678abcdul)
        passed = 0;
    dword_input = 0x0000fffful;
    if (native_replace_high_word (dword_input) != 0x5678fffful)
        passed = 0;
    dword_input = 0x89abcdeful;
    if (native_replace_high_word (dword_input) != 0x5678cdeful)
        passed = 0;

    left_dword_input = 0x00fffffeul;
    right_dword_input = 0x00000003ul;
    if (native_add_register_longs (&left_dword_input,
                                   &right_dword_input) != 0x01000002ul)
        passed = 0;

    left_dword_input = 0x00000000ul;
    right_dword_input = 0x00000001ul;
    if (native_subtract_register_longs (&left_dword_input,
                                        &right_dword_input) != 0xfffffffeul)
        passed = 0;

    print_result (passed);
    for (;;)
        {
        }
}
