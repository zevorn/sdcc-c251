#ifndef SDCC_MCS251_TEST_MEMORY_MODEL_H
#define SDCC_MCS251_TEST_MEMORY_MODEL_H

typedef unsigned long long mcs251_uint64_t;

struct mcs251_memory_model_state
{
    mcs251_uint64_t words[5];
};

void mcs251_spill_pressure (struct mcs251_memory_model_state *state);
unsigned int mcs251_spx_probe (unsigned char depth, unsigned char seed)
    __reentrant;

#endif
