#include <stdint.h>

__sfr __at (0x99) SBUF;

_Static_assert (sizeof (long long) == 8, "long long must be 64 bits");
_Static_assert (sizeof (unsigned long long) == 8,
                "unsigned long long must be 64 bits");
_Static_assert (sizeof (int64_t) == 8, "int64_t must be 64 bits");
_Static_assert (sizeof (uint64_t) == 8, "uint64_t must be 64 bits");

static volatile uint64_t unsigned_left = UINT64_C (0x1122334455667788);
static volatile uint64_t unsigned_right = UINT64_C (0x0102030405060708);
static volatile int64_t signed_left = -INT64_C (0x1122334455667788);
static volatile uint32_t conversion_value = UINT32_C (0x89abcdef);
static volatile uint8_t shift_count = 13;

static uint64_t
add_u64 (uint64_t left, uint64_t right)
{
  return left + right;
}

static uint64_t
subtract_u64 (uint64_t left, uint64_t right)
{
  return left - right;
}

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

static uint64_t
modulo_u64 (uint64_t left, uint64_t right)
{
  return left % right;
}

static int64_t
divide_s64 (int64_t left, int64_t right)
{
  return left / right;
}

static int64_t
modulo_s64 (int64_t left, int64_t right)
{
  return left % right;
}

static uint64_t
shift_left_u64 (uint64_t value, uint8_t count)
{
  return value << count;
}

static uint64_t
shift_right_u64 (uint64_t value, uint8_t count)
{
  return value >> count;
}

static int64_t
shift_right_s64 (int64_t value, uint8_t count)
{
  return value >> count;
}

static uint64_t
mixed_abi_u64 (uint8_t prefix, uint64_t value, uint16_t suffix)
{
  return value + prefix + suffix;
}

static void
print_result (uint8_t passed)
{
  const char *text = passed ? "PASS\n" : "FAIL\n";

  while (*text)
    SBUF = *text++;
}

void
main (void)
{
  uint64_t left = unsigned_left;
  uint64_t right = unsigned_right;
  int64_t negative = signed_left;
  uint8_t count = shift_count;
  uint8_t passed = 1;

  if (add_u64 (left, right) != UINT64_C (0x122436485a6c7e90))
    passed = 0;
  if (subtract_u64 (left, right) != UINT64_C (0x1020304050607080))
    passed = 0;
  if ((left & UINT64_C (0xfedcba9876543210)) !=
      UINT64_C (0x1000320054443200))
    passed = 0;
  if ((left | UINT64_C (0xfedcba9876543210)) !=
      UINT64_C (0xfffebbdc77767798))
    passed = 0;
  if ((left ^ UINT64_C (0xfedcba9876543210)) !=
      UINT64_C (0xeffe89dc23324598))
    passed = 0;
  if (!(left > right) || left <= right || left == right)
    passed = 0;

  if (shift_left_u64 (left, count) != UINT64_C (0x46688aaccef10000))
    passed = 0;
  if (shift_right_u64 (left, 17) != UINT64_C (0x0000089119a22ab3))
    passed = 0;
  if (shift_right_s64 (negative, 17) !=
      -INT64_C (0x0000089119a22ab4))
    passed = 0;

  if (multiply_u64 (UINT64_C (0x12345678), UINT64_C (0x10203)) !=
      UINT64_C (0x00001258f5c1f368))
    passed = 0;
  if (divide_u64 (left, UINT64_C (0x12345)) !=
      UINT64_C (0x00000f0f14696b50))
    passed = 0;
  if (modulo_u64 (left, UINT64_C (0x12345)) != UINT64_C (0x9af8))
    passed = 0;
  if (divide_s64 (negative, INT64_C (0x12345)) !=
      -INT64_C (0x00000f0f14696b50))
    passed = 0;
  if (modulo_s64 (negative, INT64_C (0x12345)) != -INT64_C (0x9af8))
    passed = 0;

  if (mixed_abi_u64 (0x5a, left, 0x1357) !=
      UINT64_C (0x1122334455668b39))
    passed = 0;
  if ((uint32_t)left != UINT32_C (0x55667788))
    passed = 0;
  if ((uint64_t)(int64_t)(int32_t)conversion_value !=
      UINT64_C (0xffffffff89abcdef))
    passed = 0;

  print_result (passed);
  for (;;)
    {
    }
}
