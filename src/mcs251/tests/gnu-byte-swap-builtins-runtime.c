#include <stdint.h>

__sfr __at (0x99) SBUF;

static volatile uint16_t runtime_word = UINT16_C (0x1234);
static volatile uint32_t runtime_long = UINT32_C (0x12345678);
static volatile uint64_t runtime_long_long =
  0x0123456789abcdefull;
static unsigned char evaluation_count;

static uint32_t
evaluated_long (void)
{
  evaluation_count++;
  return runtime_long;
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
  unsigned char passed = 1;

  if (__builtin_bswap16 (runtime_word) != 0x3412u ||
      __builtin_bswap32 (runtime_long) != 0x78563412ul ||
      __builtin_bswap64 (runtime_long_long) !=
        0xefcdab8967452301ull)
    passed = 0;
  if (__builtin_bswap16 (__builtin_bswap16 (runtime_word)) !=
        runtime_word ||
      __builtin_bswap32 (__builtin_bswap32 (runtime_long)) !=
        runtime_long ||
      __builtin_bswap64 (__builtin_bswap64 (runtime_long_long)) !=
        runtime_long_long)
    passed = 0;
  if (__builtin_bswap32 (evaluated_long ()) != 0x78563412ul ||
      evaluation_count != 1)
    passed = 0;

  print_result (passed);
  for (;;)
    {
    }
}
