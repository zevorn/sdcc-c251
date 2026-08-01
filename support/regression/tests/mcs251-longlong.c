/* MCS251 64-bit integer and calling-convention regression tests. */

#include <testfwk.h>

#include <stdint.h>

#if defined (__SDCC_mcs251) && defined (__SDCC_STACK_AUTO)

static volatile uint64_t unsigned_left =
  UINT64_C (0x1122334455667788);
static volatile uint64_t unsigned_right =
  UINT64_C (0x0102030405060708);
static volatile int64_t signed_left =
  -INT64_C (0x1122334455667788);
static volatile uint32_t conversion_value = UINT32_C (0x89abcdef);
static volatile uint8_t shift_count = 13;

static uint64_t
multiply_u64 (uint64_t left, uint64_t right)
{
  return left * right;
}

static uint64_t
divide_u64 (uint64_t left, uint64_t right)
{
  return left / right;
}

static int64_t
divide_s64 (int64_t left, int64_t right)
{
  return left / right;
}

static uint64_t
mixed_abi_u64 (uint8_t prefix, uint64_t value, uint16_t suffix)
{
  return value + prefix + suffix;
}

#endif

void
testMcs251LongLong (void)
{
#if defined (__SDCC_mcs251) && defined (__SDCC_STACK_AUTO)
  uint64_t left = unsigned_left;
  uint64_t right = unsigned_right;
  int64_t negative = signed_left;
  uint8_t count = shift_count;

  ASSERT (sizeof (long long) == 8);
  ASSERT (sizeof (unsigned long long) == 8);
  ASSERT (sizeof (int64_t) == 8);
  ASSERT (sizeof (uint64_t) == 8);

  ASSERT (left + right == UINT64_C (0x122436485a6c7e90));
  ASSERT (left - right == UINT64_C (0x1020304050607080));
  ASSERT ((left & UINT64_C (0xfedcba9876543210)) ==
          UINT64_C (0x1000320054443200));
  ASSERT ((left | UINT64_C (0xfedcba9876543210)) ==
          UINT64_C (0xfffebbdc77767798));
  ASSERT ((left ^ UINT64_C (0xfedcba9876543210)) ==
          UINT64_C (0xeffe89dc23324598));
  ASSERT (left > right);

  ASSERT ((left << count) == UINT64_C (0x46688aaccef10000));
  ASSERT ((left >> 17) == UINT64_C (0x0000089119a22ab3));
  ASSERT ((negative >> 17) == -INT64_C (0x0000089119a22ab4));

  ASSERT (multiply_u64 (UINT64_C (0x12345678),
                        UINT64_C (0x10203)) ==
          UINT64_C (0x00001258f5c1f368));
  ASSERT (divide_u64 (left, UINT64_C (0x12345)) ==
          UINT64_C (0x00000f0f14696b50));
  ASSERT (left % UINT64_C (0x12345) == UINT64_C (0x9af8));
  ASSERT (divide_s64 (negative, INT64_C (0x12345)) ==
          -INT64_C (0x00000f0f14696b50));
  ASSERT (negative % INT64_C (0x12345) == -INT64_C (0x9af8));

  ASSERT (mixed_abi_u64 (0x5a, left, 0x1357) ==
          UINT64_C (0x1122334455668b39));
  ASSERT ((uint32_t) left == UINT32_C (0x55667788));
  ASSERT ((uint64_t) (int64_t) (int32_t) conversion_value ==
          UINT64_C (0xffffffff89abcdef));
#endif
}
