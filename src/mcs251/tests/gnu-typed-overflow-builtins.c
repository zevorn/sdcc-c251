static volatile int gnu_overflow_int;
static volatile unsigned int gnu_overflow_uint;
static volatile long gnu_overflow_long;
static volatile unsigned long gnu_overflow_ulong;
static volatile long long gnu_overflow_long_long;
static volatile unsigned long long gnu_overflow_ulong_long;

_Static_assert (__builtin_types_compatible_p (
                  __typeof__ (__builtin_sadd_overflow (
                    1, 2, (int *)0)), _Bool),
                "sadd_overflow returns _Bool");
_Static_assert (__builtin_types_compatible_p (
                  __typeof__ (__builtin_uaddl_overflow (
                    1UL, 2UL, (unsigned long *)0)), _Bool),
                "uaddl_overflow returns _Bool");
_Static_assert (__builtin_types_compatible_p (
                  __typeof__ (__builtin_smulll_overflow (
                    1LL, 2LL, (long long *)0)), _Bool),
                "smulll_overflow returns _Bool");

_Bool
gnu_typed_overflow_builtin_probe (void)
{
  return
    __builtin_sadd_overflow (
      gnu_overflow_int, 1, (int *)&gnu_overflow_int) ||
    __builtin_saddl_overflow (
      gnu_overflow_long, 1L, (long *)&gnu_overflow_long) ||
    __builtin_saddll_overflow (
      gnu_overflow_long_long, 1LL,
      (long long *)&gnu_overflow_long_long) ||
    __builtin_uadd_overflow (
      gnu_overflow_uint, 1U, (unsigned int *)&gnu_overflow_uint) ||
    __builtin_uaddl_overflow (
      gnu_overflow_ulong, 1UL, (unsigned long *)&gnu_overflow_ulong) ||
    __builtin_uaddll_overflow (
      gnu_overflow_ulong_long, 1ULL,
      (unsigned long long *)&gnu_overflow_ulong_long) ||
    __builtin_ssub_overflow (
      gnu_overflow_int, 1, (int *)&gnu_overflow_int) ||
    __builtin_ssubl_overflow (
      gnu_overflow_long, 1L, (long *)&gnu_overflow_long) ||
    __builtin_ssubll_overflow (
      gnu_overflow_long_long, 1LL,
      (long long *)&gnu_overflow_long_long) ||
    __builtin_usub_overflow (
      gnu_overflow_uint, 1U, (unsigned int *)&gnu_overflow_uint) ||
    __builtin_usubl_overflow (
      gnu_overflow_ulong, 1UL, (unsigned long *)&gnu_overflow_ulong) ||
    __builtin_usubll_overflow (
      gnu_overflow_ulong_long, 1ULL,
      (unsigned long long *)&gnu_overflow_ulong_long) ||
    __builtin_smul_overflow (
      gnu_overflow_int, 2, (int *)&gnu_overflow_int) ||
    __builtin_smull_overflow (
      gnu_overflow_long, 2L, (long *)&gnu_overflow_long) ||
    __builtin_smulll_overflow (
      gnu_overflow_long_long, 2LL,
      (long long *)&gnu_overflow_long_long) ||
    __builtin_umul_overflow (
      gnu_overflow_uint, 2U, (unsigned int *)&gnu_overflow_uint) ||
    __builtin_umull_overflow (
      gnu_overflow_ulong, 2UL, (unsigned long *)&gnu_overflow_ulong) ||
    __builtin_umulll_overflow (
      gnu_overflow_ulong_long, 2ULL,
      (unsigned long long *)&gnu_overflow_ulong_long);
}
