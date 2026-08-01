/* Wide bit-field regression tests for the MCS51 family. */

#include <testfwk.h>

#if (defined (__SDCC_mcs51) || defined (__SDCC_mcs251)) && \
  defined (__SDCC_STACK_AUTO)

struct fields32
{
  unsigned long low : 7;
  unsigned long high : 25;
};

struct fields64
{
  unsigned long long low : 31;
  unsigned long long high : 33;
};

struct signed_field64
{
  signed long long value : 52;
  unsigned long long padding : 12;
};

static struct fields32 value32;
static struct fields64 value64;
static struct signed_field64 signed_value64;

#endif

void
testWideBitfields (void)
{
#if (defined (__SDCC_mcs51) || defined (__SDCC_mcs251)) && \
  defined (__SDCC_STACK_AUTO)
  value32.low = 0x55ul;
  value32.high = 0x01abcdeful;
  ASSERT (value32.low == 0x55ul);
  ASSERT (value32.high == 0x01abcdeful);

  value64.low = 0x5abcdef0ull;
  value64.high = 0x1abcde123ull;
  ASSERT (value64.low == 0x5abcdef0ull);
  ASSERT (value64.high == 0x1abcde123ull);

  signed_value64.value = -0x123456789abll;
  signed_value64.padding = 0xabcu;
  ASSERT (signed_value64.value == -0x123456789abll);
  ASSERT (signed_value64.padding == 0xabcu);
#endif
}
