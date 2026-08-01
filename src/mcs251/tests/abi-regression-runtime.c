#include <stdarg.h>
#include <string.h>

__sfr __at (0x99) SBUF;

struct abi_sample
{
    unsigned int value;
    unsigned char tag;
};

static struct abi_sample sample = {0x1234, 0x5a};
static __pdata unsigned char paged_values[] = {0x12, 0x34, 0x56, 0x78};
static volatile float float_value = 3.875f;
static volatile unsigned int word_addend;
static volatile unsigned int captured_word_result;
static volatile unsigned long long captured_wide_result;
static unsigned long bitwise_words[4];

static unsigned int
return_word_result (void)
{
    return 0x4567;
}

static unsigned long long
return_wide_result (void)
{
    return 0x8877665544332211ULL;
}

static unsigned int
accept_sample (struct abi_sample value)
{
    return value.value + value.tag;
}

static unsigned char
compare_va_copies (va_list arguments)
{
    va_list copy;
    unsigned int first;
    unsigned int copied_first;
    unsigned int second;
    unsigned int copied_second;

    va_copy (copy, arguments);
    first = va_arg (arguments, unsigned int);
    copied_first = va_arg (copy, unsigned int);
    second = va_arg (arguments, unsigned int);
    copied_second = va_arg (copy, unsigned int);
    va_end (copy);
    return first == 0x1357 && copied_first == 0x1357 &&
           second == 0x2468 && copied_second == 0x2468;
}

static unsigned char
variadic_probe (unsigned char marker, ...)
{
    va_list arguments;
    unsigned char passed;

    va_start (arguments, marker);
    passed = compare_va_copies (arguments);
    va_end (arguments);
    return passed && marker == 0xa5;
}

static unsigned char
copy_stack_probe (void) __reentrant
{
    unsigned char source[] = {0x0a, 0x14, 0x1e};
    unsigned char destination[sizeof source];

    memcpy (destination, source, sizeof source);
    return destination[0] == 0x0a && destination[1] == 0x14 &&
           destination[2] == 0x1e;
}

static unsigned char
bitwise_overlap_probe (void)
{
    unsigned char i = 0;
    unsigned long *words = bitwise_words;
    unsigned long temporary;

    words[0] = 0x44cbd21dUL;
    words[1] = 0xa5e56371UL;
    words[2] = 0x208188f4UL;
    words[3] = 0xea3fa7feUL;

    /* This is Serpent S-box 7.  It gives the allocator enough live values
       to place a binary result in [r2,r1,r3,r4] while its right operand is
       still in [r1,r2,r3,r4].  A low-to-high emitter must defer the first
       write or it destroys the second source byte. */
    temporary = words[i + 1];
    words[i + 1] |= words[i + 2];
    words[i + 1] ^= words[i + 3];
    temporary ^= words[i + 2];
    words[i + 2] ^= words[i + 1];
    words[i + 3] |= temporary;
    words[i + 3] &= words[i + 0];
    temporary ^= words[i + 2];
    words[i + 3] ^= words[i + 1];
    words[i + 1] |= temporary;
    words[i + 1] ^= words[i + 0];
    words[i + 0] |= temporary;
    words[i + 0] ^= words[i + 2];
    words[i + 1] ^= temporary;
    words[i + 2] ^= words[i + 1];
    words[i + 1] &= words[i + 0];
    words[i + 1] ^= temporary;
    words[i + 2] = ~words[i + 2];
    words[i + 2] |= words[i + 0];
    temporary ^= words[i + 2];
    words[i + 2] = words[i + 1];
    words[i + 1] = words[i + 3];
    words[i + 3] = words[i + 0];
    words[i + 0] = temporary;

    return words[0] == 0x3b9094e6UL &&
           words[1] == 0x0b918e16UL &&
           words[2] == 0xeb3f3d7aUL &&
           words[3] == 0x81a43b80UL;
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
    unsigned char *flat_paged_values = (unsigned char *)paged_values;
    unsigned char passed = 1;

    if (accept_sample (sample) != 0x128e)
        passed = 0;

    if (!(float_value == 3.875f) || float_value != 3.875f ||
        !(float_value >= 3.875f) || !(float_value <= 3.875f))
        passed = 0;

    if (!variadic_probe (0xa5, 0x1357, 0x2468))
        passed = 0;

    if (!copy_stack_probe ())
        passed = 0;

    if (!bitwise_overlap_probe ())
        passed = 0;

    captured_wide_result = return_wide_result ();
    if (captured_wide_result != 0x8877665544332211ULL)
        passed = 0;

    /* In the large models, the return value occupies A:B while the volatile
       addend needs @DPX.  Loading its low byte must preserve the high return
       byte held in B. */
    word_addend = 0x1234;
    captured_word_result = return_word_result () + word_addend;
    if (captured_word_result != 0x579b)
        passed = 0;

    if (flat_paged_values[0] != 0x12 || flat_paged_values[1] != 0x34 ||
        flat_paged_values[2] != 0x56 || flat_paged_values[3] != 0x78)
        passed = 0;
    flat_paged_values[2] = 0xa9;
    if (paged_values[2] != 0xa9)
        passed = 0;

    print_result (passed);
    for (;;)
        {
        }
}
