__sfr __at (0x99) SBUF;

union endian_word
{
    unsigned int value;
    unsigned char bytes[2];
};

union endian_dword
{
    unsigned long value;
    unsigned char bytes[4];
};

union endian_qword
{
    unsigned long long value;
    unsigned char bytes[8];
};

static volatile __data union endian_word direct_word = {0x1234u};
static volatile __data union endian_dword direct_dword = {0x12345678ul};
static volatile __data union endian_qword direct_qword = {
    0x0123456789abcdefull,
};
static volatile __xdata union endian_word external_word = {0x5678u};
static volatile __xdata union endian_dword external_dword = {0x89abcdeful};
static volatile __pdata union endian_word paged_word = {0x3456u};
static const __code union endian_word code_word = {0x789au};

unsigned char
__sdcc_external_startup (void)
{
    return 0;
}

static unsigned char
direct_layout_is_big_endian (void)
{
    static const unsigned char qword_bytes[8] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    };
    unsigned char passed = 1;
    unsigned char index;

    passed &= direct_word.value == 0x1234u;
    passed &= direct_word.bytes[0] == 0x12;
    passed &= direct_word.bytes[1] == 0x34;
    passed &= direct_dword.value == 0x12345678ul;
    passed &= direct_dword.bytes[0] == 0x12;
    passed &= direct_dword.bytes[1] == 0x34;
    passed &= direct_dword.bytes[2] == 0x56;
    passed &= direct_dword.bytes[3] == 0x78;
    passed &= direct_qword.value == 0x0123456789abcdefull;
    for (index = 0; index < sizeof qword_bytes; ++index)
        passed &= direct_qword.bytes[index] == qword_bytes[index];
    return passed;
}

static unsigned char
external_layout_is_big_endian (void)
{
    unsigned char passed = 1;

    passed &= external_word.value == 0x5678u;
    passed &= external_word.bytes[0] == 0x56;
    passed &= external_word.bytes[1] == 0x78;
    passed &= external_dword.value == 0x89abcdeful;
    passed &= external_dword.bytes[0] == 0x89;
    passed &= external_dword.bytes[1] == 0xab;
    passed &= external_dword.bytes[2] == 0xcd;
    passed &= external_dword.bytes[3] == 0xef;
    return passed;
}

static unsigned char
stack_layout_is_big_endian (void) __reentrant
{
    volatile union endian_word word;
    volatile union endian_dword dword;
    unsigned char passed = 1;

    word.value = 0x9abcu;
    dword.value = 0x76543210ul;
    passed &= word.value == 0x9abcu;
    passed &= word.bytes[0] == 0x9a;
    passed &= word.bytes[1] == 0xbc;
    passed &= dword.value == 0x76543210ul;
    passed &= dword.bytes[0] == 0x76;
    passed &= dword.bytes[1] == 0x54;
    passed &= dword.bytes[2] == 0x32;
    passed &= dword.bytes[3] == 0x10;
    return passed;
}

static unsigned char
near_pointer_layout_is_big_endian (
    volatile union endian_word __data *value)
{
    unsigned char passed = value->value == 0x1234u;

    passed &= value->bytes[0] == 0x12;
    passed &= value->bytes[1] == 0x34;
    value->value = 0x2468u;
    passed &= value->bytes[0] == 0x24;
    passed &= value->bytes[1] == 0x68;
    return passed;
}

static unsigned char
paged_pointer_layout_is_big_endian (
    volatile union endian_word __pdata *value)
{
    unsigned char passed = value->value == 0x3456u;

    passed &= value->bytes[0] == 0x34;
    passed &= value->bytes[1] == 0x56;
    value->value = 0x1357u;
    passed &= value->bytes[0] == 0x13;
    passed &= value->bytes[1] == 0x57;
    return passed;
}

static unsigned char
code_pointer_layout_is_big_endian (
    const union endian_word __code *value)
{
    return value->value == 0x789au &&
           value->bytes[0] == 0x78 && value->bytes[1] == 0x9a;
}

static unsigned char
generic_pointer_layout_is_big_endian (
    volatile union endian_word *value)
{
    unsigned char passed = value->value == 0x5678u;

    passed &= value->bytes[0] == 0x56;
    passed &= value->bytes[1] == 0x78;
    value->value = 0x4567u;
    passed &= value->bytes[0] == 0x45;
    passed &= value->bytes[1] == 0x67;
    return passed;
}

static unsigned char
stack_parameter_layout_is_big_endian (unsigned char marker,
                                      volatile unsigned int value) __reentrant
{
    volatile unsigned char __xdata *bytes =
        (volatile unsigned char __xdata *)&value;

    return marker == 0xa5 && value == 0x1234u &&
           bytes[0] == 0x12 && bytes[1] == 0x34;
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
    unsigned char direct_passed = direct_layout_is_big_endian ();
    unsigned char external_passed = external_layout_is_big_endian ();
    unsigned char stack_passed = stack_layout_is_big_endian ();
    unsigned char near_passed = near_pointer_layout_is_big_endian (
        &direct_word);
    unsigned char paged_passed = paged_pointer_layout_is_big_endian (
        &paged_word);
    unsigned char code_passed = code_pointer_layout_is_big_endian (
        &code_word);
    unsigned char generic_passed = generic_pointer_layout_is_big_endian (
        (volatile union endian_word *)&external_word);
    unsigned char parameter_passed = stack_parameter_layout_is_big_endian (
        0xa5, 0x1234u);

    if (!direct_passed)
        SBUF = 'D';
    if (!external_passed)
        SBUF = 'X';
    if (!stack_passed)
        SBUF = 'S';
    if (!near_passed)
        SBUF = 'N';
    if (!paged_passed)
        SBUF = 'P';
    if (!code_passed)
        SBUF = 'C';
    if (!generic_passed)
        SBUF = 'G';
    if (!parameter_passed)
        SBUF = 'A';
    print_result (direct_passed && external_passed && stack_passed &&
                  near_passed && paged_passed && code_passed &&
                  generic_passed && parameter_passed);
    for (;;)
        {
        }
}
