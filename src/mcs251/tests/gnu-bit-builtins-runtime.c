__sfr __at (0x99) SBUF;

static volatile unsigned int runtime_word = 0x8100u;
static volatile unsigned long runtime_long = 0x81000000ul;
static volatile unsigned long long runtime_long_long =
  0x8100000000000000ull;
static unsigned int evaluation_count;

static unsigned int
evaluated_word (void)
{
  evaluation_count++;
  return runtime_word;
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

  if (__builtin_clz (runtime_word) != 0 ||
      __builtin_clzl (runtime_long) != 0 ||
      __builtin_clzll (runtime_long_long) != 0)
    passed = 0;
  if (__builtin_ctz (runtime_word) != 8 ||
      __builtin_ctzl (runtime_long) != 24 ||
      __builtin_ctzll (runtime_long_long) != 56)
    passed = 0;
  if (__builtin_popcount (runtime_word) != 2 ||
      __builtin_popcountl (runtime_long) != 2 ||
      __builtin_popcountll (runtime_long_long) != 2)
    passed = 0;
  if (__builtin_ffs (runtime_word) != 9 ||
      __builtin_ffsl (runtime_long) != 25 ||
      __builtin_ffsll (runtime_long_long) != 57 ||
      __builtin_ffs (0) != 0 ||
      __builtin_ffs (-2) != 2)
    passed = 0;
  if (__builtin_popcount (evaluated_word ()) != 2 ||
      evaluation_count != 1)
    passed = 0;

  print_result (passed);
  for (;;)
    {
    }
}
