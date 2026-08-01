volatile unsigned char gnu_expect_value_count;
volatile unsigned char gnu_expect_hint_count;

typedef _Bool gnu_bool;

_Static_assert (__builtin_expect (7, 1) == 7,
                "__builtin_expect must return its first argument");
_Static_assert (__builtin_types_compatible_p (
                  __typeof__ (__builtin_expect ((unsigned char)1, 1)),
                  long),
                "__builtin_expect must have type long");

long
gnu_expect_side_effects (void)
{
  return __builtin_expect (++gnu_expect_value_count,
                           ++gnu_expect_hint_count);
}

int
gnu_likely (int value)
{
  if (__builtin_expect (!!value, 1) != 0L)
    return 1;

  return 0;
}

int
gnu_unlikely (int value)
{
  if (__builtin_expect (!!value, 0) != 0L)
    return 1;

  return 0;
}

int
gnu_likely_boolean_cast (int value)
{
  if (__builtin_expect ((gnu_bool) !!(value), 1) != 0L)
    return 1;

  return 0;
}
