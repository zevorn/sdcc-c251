#if GNU_TYPED_OVERFLOW_INVALID_CASE == 1
int
gnu_typed_overflow_signed_result (unsigned int *result)
{
  return __builtin_sadd_overflow (1, 2, result);
}
#elif GNU_TYPED_OVERFLOW_INVALID_CASE == 2
int
gnu_typed_overflow_unsigned_result (int *result)
{
  return __builtin_uadd_overflow (1U, 2U, result);
}
#elif GNU_TYPED_OVERFLOW_INVALID_CASE == 3
int
gnu_typed_overflow_long_result (int *result)
{
  return __builtin_ssubl_overflow (1L, 2L, result);
}
#elif GNU_TYPED_OVERFLOW_INVALID_CASE == 4
int
gnu_typed_overflow_nonpointer (void)
{
  return __builtin_umull_overflow (1UL, 2UL, 0UL);
}
#elif GNU_TYPED_OVERFLOW_INVALID_CASE == 5
int
gnu_typed_overflow_const_result (const long long *result)
{
  return __builtin_smulll_overflow (1LL, 2LL, result);
}
#elif GNU_TYPED_OVERFLOW_INVALID_CASE == 6
int
gnu_typed_overflow_atomic_result (_Atomic unsigned long long *result)
{
  return __builtin_umulll_overflow (1ULL, 2ULL, result);
}
#elif GNU_TYPED_OVERFLOW_INVALID_CASE == 7
int
gnu_typed_overflow_too_few (unsigned int *result)
{
  return __builtin_usub_overflow (1U, result);
}
#elif GNU_TYPED_OVERFLOW_INVALID_CASE == 8
int
gnu_typed_overflow_too_many (unsigned int *result)
{
  return __builtin_usub_overflow (1U, 2U, result, 3U);
}
#elif GNU_TYPED_OVERFLOW_INVALID_CASE == 9
int
gnu_typed_overflow_short_result (short *result)
{
  return __builtin_sadd_overflow (1, 2, result);
}
#elif GNU_TYPED_OVERFLOW_INVALID_CASE == 10
enum gnu_typed_overflow_enum
{
  GNU_TYPED_OVERFLOW_ZERO
};

int
gnu_typed_overflow_enum_result (enum gnu_typed_overflow_enum *result)
{
  return __builtin_sadd_overflow (1, 2, result);
}
#elif GNU_TYPED_OVERFLOW_INVALID_CASE == 11
int
gnu_typed_overflow_code_result (int __code *result)
{
  return __builtin_sadd_overflow (1, 2, result);
}
#endif
