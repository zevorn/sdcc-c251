#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

__sfr __at (0x99) SBUF;

#if defined (__SDCC_mcs251)
_Static_assert (sizeof (size_t) == 4, "MCS251 size_t width");
_Static_assert (__builtin_types_compatible_p (size_t, unsigned long),
                "MCS251 size_t type");
static volatile size_t runtime_size = UINT32_C (0x01020304);
#else
_Static_assert (sizeof (size_t) == 2, "MCS51 size_t width");
_Static_assert (__builtin_types_compatible_p (size_t, unsigned int),
                "MCS51 size_t type");
static volatile size_t runtime_size = UINT16_C (0x1234);
#endif

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
  char buffer[4];
  size_t result = runtime_size;
  unsigned char passed = SIZE_MAX == __SIZE_MAX__;

#if defined (__SDCC_mcs251)
  result += UINT32_C (0x10203040);
  if (result != UINT32_C (0x11223344))
    passed = 0;
#else
  result += UINT16_C (0x1111);
  if (result != UINT16_C (0x2345))
    passed = 0;
#endif

  memcpy (buffer, "abc", sizeof (buffer));
  if (strlen (buffer) != 3 || strnlen (buffer, SIZE_MAX) != 3)
    passed = 0;

#if defined (__SDCC_mcs251)
  if (calloc ((size_t)UINT32_C (0x80000000), 2) != 0)
    passed = 0;
#else
  if (calloc ((size_t)UINT16_C (0x8000), 2) != 0)
    passed = 0;
#endif

  print_result (passed);
  for (;;)
    {
    }
}
