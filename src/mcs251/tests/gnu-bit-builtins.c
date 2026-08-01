static volatile unsigned int gnu_bit_word = 0x8100u;
static volatile unsigned long gnu_bit_long = 0x81000000ul;
static volatile unsigned long long gnu_bit_long_long =
  0x8100000000000000ull;

_Static_assert (__builtin_clz (1u) == 15,
                "clz uses the unsigned int width");
_Static_assert (__builtin_clzl (1ul) == 31,
                "clzl uses the unsigned long width");
_Static_assert (__builtin_clzll (1ull) == 63,
                "clzll uses the unsigned long long width");
_Static_assert (__builtin_ctz (0x8000u) == 15,
                "ctz counts unsigned int trailing zeroes");
_Static_assert (__builtin_ctzl (0x80000000ul) == 31,
                "ctzl counts unsigned long trailing zeroes");
_Static_assert (__builtin_ctzll (0x8000000000000000ull) == 63,
                "ctzll counts unsigned long long trailing zeroes");
_Static_assert (__builtin_popcount (0xa55au) == 8,
                "popcount counts unsigned int bits");
_Static_assert (__builtin_popcountl (0x80010001ul) == 3,
                "popcountl counts unsigned long bits");
_Static_assert (__builtin_popcountll (0x8000000100000001ull) == 3,
                "popcountll counts unsigned long long bits");
_Static_assert (__builtin_ffs (0) == 0 &&
                __builtin_ffs (0x0100) == 9 &&
                __builtin_ffs (-1) == 1,
                "ffs uses one-origin indexing for signed values");
_Static_assert (__builtin_ffsl (0x01000000l) == 25,
                "ffsl uses the signed long width");
_Static_assert (__builtin_ffsll (0x0100000000000000ll) == 57,
                "ffsll uses the signed long long width");

int
gnu_bit_builtin_probe (void)
{
  return __builtin_clz (gnu_bit_word) +
    __builtin_clzl (gnu_bit_long) +
    __builtin_clzll (gnu_bit_long_long) +
    __builtin_ctz (gnu_bit_word) +
    __builtin_ctzl (gnu_bit_long) +
    __builtin_ctzll (gnu_bit_long_long) +
    __builtin_popcount (gnu_bit_word) +
    __builtin_popcountl (gnu_bit_long) +
    __builtin_popcountll (gnu_bit_long_long) +
    __builtin_ffs (gnu_bit_word) +
    __builtin_ffsl (gnu_bit_long) +
    __builtin_ffsll (gnu_bit_long_long);
}
