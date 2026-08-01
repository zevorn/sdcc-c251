#include "memory-model.h"

void
mcs251_spill_pressure (struct mcs251_memory_model_state *state)
{
    mcs251_uint64_t x0 = state->words[0];
    mcs251_uint64_t x1 = state->words[1];
    mcs251_uint64_t x2 = state->words[2];
    mcs251_uint64_t x3 = state->words[3];
    mcs251_uint64_t x4 = state->words[4];

    x0 ^= x4;
    x4 ^= x3;
    x2 ^= x1;
    x0 ^= ~x1 & x2;
    x1 ^= ~x2 & x3;
    x2 ^= ~x3 & x4;
    x3 ^= ~x4 & x0;
    x4 ^= ~x0 & x1;

    state->words[0] = x0;
    state->words[1] = x1;
    state->words[2] = x2;
    state->words[3] = x3;
    state->words[4] = x4;
}

/* Seven live frames place more than 0x100 bytes on the hardware stack.  This
   catches any MCS251 lowering that addresses automatic objects through only the
   low SP byte after SPX crosses a 256-byte boundary. */
unsigned int
mcs251_spx_probe (unsigned char depth, unsigned char seed) __reentrant
{
    volatile unsigned char local[48];
    unsigned char i;

    for (i = 0; i < sizeof (local); ++i)
        local[i] = seed + i;

    if (depth)
        return local[depth] +
               mcs251_spx_probe (depth - 1, seed + 3) +
               local[47 - depth];

    return local[0] + local[47];
}

/* Keep a byte on the extended stack while exercising the classic shift
   peephole.  MCS251 indexed stack operands are valid for MOV, but not as an
   ADD source; this catches an inherited MCS-51 peephole accidentally
   creating "add a,@spx+dis16". */
unsigned char
mcs251_spx_shift_probe (unsigned char value) __reentrant
{
    volatile unsigned char stack_value = value;

    return stack_value << 1;
}
