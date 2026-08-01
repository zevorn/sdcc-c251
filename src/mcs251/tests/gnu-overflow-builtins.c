static volatile int gnu_overflow_int;
static volatile unsigned long gnu_overflow_ulong;
static volatile long long gnu_overflow_long_long;

_Static_assert (__builtin_types_compatible_p (
                  __typeof__ (__builtin_add_overflow (
                    1, 2, (int *)0)), _Bool),
                "add_overflow returns _Bool");
_Static_assert (__builtin_types_compatible_p (
                  __typeof__ (__builtin_sub_overflow (
                    1L, 2U, (long *)0)), _Bool),
                "sub_overflow returns _Bool");
_Static_assert (__builtin_types_compatible_p (
                  __typeof__ (__builtin_mul_overflow (
                    1ULL, 2, (unsigned long long *)0)), _Bool),
                "mul_overflow returns _Bool");

_Bool
gnu_overflow_builtin_probe (void)
{
  return __builtin_add_overflow (
           gnu_overflow_int, gnu_overflow_ulong,
           (unsigned char *)&gnu_overflow_int) ||
    __builtin_sub_overflow (
      gnu_overflow_long_long, gnu_overflow_int,
      (long long *)&gnu_overflow_long_long) ||
    __builtin_mul_overflow (
      gnu_overflow_ulong, gnu_overflow_long_long,
      (unsigned long *)&gnu_overflow_ulong);
}
