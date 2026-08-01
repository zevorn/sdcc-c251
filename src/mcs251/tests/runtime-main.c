#include <setjmp.h>
#include <stdlib.h>

__sfr __at (0x99) SBUF;
__xdata __at (0x010020) volatile unsigned char high_data;

struct mcs251_index_pair
{
    unsigned char first;
    unsigned char second;
};

static __xdata struct mcs251_index_pair indexed_pairs[40];
static volatile signed char relative_index = -3;

#ifndef MCS251_SKIP_SETJMP
static __xdata jmp_buf jump_buffer;
static volatile unsigned char deep_call_count;

/* Reach setjmp with SPX above 0x00ff, then verify that longjmp restores both
   bytes of SPX and both bytes of its result. */
static int
deep_setjmp (unsigned char depth) __reentrant
{
    volatile unsigned char padding[48];
    int resumed;

    ++deep_call_count;
    padding[0] = depth;
    padding[47] = depth + 1;
    if (depth)
        return deep_setjmp (depth - 1) + padding[0] - depth;

    resumed = setjmp (jump_buffer);
    if (!resumed)
        longjmp (jump_buffer, 0x1234);
    return resumed + padding[47] - 1;
}
#endif

#ifndef MCS251_SKIP_BSEARCH
static unsigned char values[] = {1, 3, 5, 7};
static unsigned char key = 5;

static int
compare_byte (const void *left, const void *right) __reentrant
{
    const unsigned char *left_byte = left;
    const unsigned char *right_byte = right;

    return (int)*left_byte - (int)*right_byte;
}
#endif

unsigned char
__sdcc_external_startup (void)
{
    return 0;
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

static unsigned int
native_and16 (unsigned int left, unsigned int right)
{
    return left & right;
}

static unsigned int
native_or16 (unsigned int left, unsigned int right)
{
    return left | right;
}

static unsigned int
native_xor16 (unsigned int left, unsigned int right)
{
    return left ^ right;
}

static unsigned long
native_xor32 (unsigned long left, unsigned long right)
{
    return left ^ right;
}

static unsigned int
small_model_spx_probe (unsigned char depth, unsigned char seed) __reentrant
{
    volatile unsigned char local[48];
    unsigned char i;

    for (i = 0; i < sizeof (local); ++i)
        local[i] = seed + i;

    if (depth)
        return local[depth] +
               small_model_spx_probe (depth - 1, seed + 3) +
               local[47 - depth];

    return local[0] + local[47];
}

void
main (void)
{
    struct mcs251_index_pair __xdata *middle_pair;
    unsigned char __xdata *far_pointer;
    unsigned char *generic_pointer;
    unsigned char __xdata *boundary_pointer;
#ifndef MCS251_SKIP_DIVISION
    volatile unsigned long dividend = 100000UL;
    volatile unsigned long quotient;
#endif
#ifndef MCS251_SKIP_FLOAT
    volatile float parsed;
#endif
#ifndef MCS251_SKIP_BSEARCH
    unsigned char *found;
#endif
#ifndef MCS251_SKIP_SETJMP
    int resumed;
#endif
    unsigned char passed = 1;

    middle_pair = &indexed_pairs[20];
    middle_pair[relative_index].second = 0x5a;
    if (indexed_pairs[17].second != 0x5a)
        passed = 0;

    far_pointer = &high_data;
    *far_pointer = 0x5a;
    generic_pointer = (unsigned char *)far_pointer;
    if (*generic_pointer != 0x5a)
        passed = 0;

    boundary_pointer = (unsigned char __xdata *)0x01ffffUL;
    boundary_pointer[0] = 0xa5;
    boundary_pointer[1] = 0x3c;
    if (boundary_pointer[0] != 0xa5 || boundary_pointer[1] != 0x3c)
        passed = 0;

    if (native_and16 (0xa55a, 0x0ff0) != 0x0550 ||
        native_or16 (0xa55a, 0x0ff0) != 0xaffa ||
        native_xor16 (0xa55a, 0x0ff0) != 0xaaaa ||
        native_xor32 (0x12345678UL, 0x0ff00ff0UL) != 0x1dc45988UL ||
        small_model_spx_probe (6, 5) != 525)
        passed = 0;

#ifndef MCS251_SKIP_DIVISION
    quotient = dividend / 10UL;
    if (quotient != 10000UL)
        passed = 0;
#endif

#ifndef MCS251_SKIP_FLOAT
    parsed = atof ("12.5");
    if (parsed < 12.49f || parsed > 12.51f)
        passed = 0;
#endif

#ifndef MCS251_SKIP_BSEARCH
    found = bsearch (&key, values, sizeof (values), sizeof (values[0]),
                     compare_byte);
    if (!found || *found != key)
        passed = 0;
#endif

#ifndef MCS251_SKIP_SETJMP
    resumed = setjmp (jump_buffer);
    if (resumed == 0)
        longjmp (jump_buffer, 0);
    deep_call_count = 0;
    if (resumed != 1 || deep_setjmp (6) != 0x1234 ||
        deep_call_count != 7)
        passed = 0;
#endif

    print_result (passed);
    for (;;)
        {
        }
}
