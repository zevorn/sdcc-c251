long
gnu_expect_pointer_value (int *value)
{
  return __builtin_expect (value, 0);
}

long
gnu_expect_pointer_hint (int value, int *hint)
{
  return __builtin_expect (value, hint);
}
