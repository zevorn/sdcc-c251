#include "memory-model.h"

__sfr __at (0x99) SBUF;

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

static void
print_hex_byte (unsigned char value)
{
    unsigned char digit = value >> 4;

    SBUF = digit < 10 ? digit + '0' : digit + ('a' - 10);
    digit = value & 0x0f;
    SBUF = digit < 10 ? digit + '0' : digit + ('a' - 10);
}

static void
dump_state (struct mcs251_memory_model_state *state)
{
    unsigned char __xdata *bytes =
        (unsigned char __xdata *)state;
    unsigned char i;

    for (i = 0; i < sizeof (*state); ++i)
        {
            print_hex_byte (bytes[i]);
            SBUF = (i & 7) == 7 ? '\n' : ' ';
        }
}

void
main (void)
{
    struct mcs251_memory_model_state state = {{
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x1020304050607080ULL,
    }};
    unsigned char passed;

    mcs251_spill_pressure (&state);
    passed =
        state.words[0] == 0x10017003d0c1f407ULL &&
        state.words[1] == 0xf6e9dac936651201ULL &&
        state.words[2] == 0xe1c287a42d4e0be8ULL &&
        state.words[3] == 0x8877465784b38617ULL &&
        state.words[4] == 0x7ebfdcdd32775091ULL &&
        mcs251_spx_probe (6, 5) == 525;

    if (!passed)
        dump_state (&state);
    print_result (passed);
    for (;;)
        {
        }
}
