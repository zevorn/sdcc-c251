enum gnu_overflow_enum
{
  GNU_OVERFLOW_ZERO
};

#if GNU_OVERFLOW_INVALID_CASE == 1
int
gnu_overflow_builtin_nonpointer (void)
{
  return __builtin_add_overflow (1, 2, 3);
}
#elif GNU_OVERFLOW_INVALID_CASE == 2
int
gnu_overflow_builtin_float_result (float *result)
{
  return __builtin_sub_overflow (1, 2, result);
}
#elif GNU_OVERFLOW_INVALID_CASE == 3
int
gnu_overflow_builtin_bool_result (_Bool *result)
{
  return __builtin_mul_overflow (1, 2, result);
}
#elif GNU_OVERFLOW_INVALID_CASE == 4
int
gnu_overflow_builtin_enum_result (enum gnu_overflow_enum *result)
{
  return __builtin_add_overflow (1, 2, result);
}
#elif GNU_OVERFLOW_INVALID_CASE == 5
int
gnu_overflow_builtin_const_result (const int *result)
{
  return __builtin_add_overflow (1, 2, result);
}
#elif GNU_OVERFLOW_INVALID_CASE == 6
int
gnu_overflow_builtin_atomic_result (_Atomic int *result)
{
  return __builtin_add_overflow (1, 2, result);
}
#elif GNU_OVERFLOW_INVALID_CASE == 7
int
gnu_overflow_builtin_float_operand (int *result)
{
  return __builtin_sub_overflow (1.0f, 2, result);
}
#elif GNU_OVERFLOW_INVALID_CASE == 8
int
gnu_overflow_builtin_too_few (int *result)
{
  return __builtin_mul_overflow (1, result);
}
#elif GNU_OVERFLOW_INVALID_CASE == 9
int
gnu_overflow_builtin_too_many (int *result)
{
  return __builtin_mul_overflow (1, 2, result, 3);
}
#endif
