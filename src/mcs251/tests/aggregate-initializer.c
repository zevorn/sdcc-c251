struct initializer_pair
{
  int first;
  int second;
};

struct initializer_outer
{
  int prefix;
  union
  {
    int primary;
    int alternate;
  };
  struct initializer_pair tail;
};

struct initializer_outer initializer_value = {1, 2, {3, 4}};

char initializer_target;
char * const initialized_data_pointer = &initializer_target;

int
initialized_function_symbol_is_nonnull (void)
{
  return initialized_function_symbol_is_nonnull != 0;
}
