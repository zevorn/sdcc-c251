#include <stdint.h>

static volatile uint16_t gnu_bswap_word = UINT16_C (0x1234);
static volatile uint32_t gnu_bswap_long = UINT32_C (0x12345678);
static volatile uint64_t gnu_bswap_long_long =
  0x0123456789abcdefull;

_Static_assert (__builtin_bswap16 (0x1234u) == 0x3412u,
                "bswap16 reverses two bytes");
_Static_assert (__builtin_bswap32 (0x12345678ul) == 0x78563412ul,
                "bswap32 reverses four bytes");
_Static_assert (
  __builtin_bswap64 (0x0123456789abcdefull) ==
    0xefcdab8967452301ull,
  "bswap64 reverses eight bytes");
_Static_assert (__builtin_types_compatible_p (
                  __typeof__ (__builtin_bswap16 (0)), uint16_t),
                "bswap16 returns uint16_t");
_Static_assert (__builtin_types_compatible_p (
                  __typeof__ (__builtin_bswap32 (0)), uint32_t),
                "bswap32 returns uint32_t");
_Static_assert (__builtin_types_compatible_p (
                  __typeof__ (__builtin_bswap64 (0)), uint64_t),
                "bswap64 returns uint64_t");

unsigned long long
gnu_byte_swap_builtin_probe (void)
{
  return __builtin_bswap16 (gnu_bswap_word) ^
    __builtin_bswap32 (gnu_bswap_long) ^
    __builtin_bswap64 (gnu_bswap_long_long);
}
