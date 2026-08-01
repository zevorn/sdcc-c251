typedef _Bool gnu_bool;

#define GNU_MIN(first, second) \
  ({ \
    __typeof__ (first) gnu_first = (first); \
    __typeof__ (second) gnu_second = (second); \
    gnu_first < gnu_second ? gnu_first : gnu_second; \
  })

int
gnu_statement_expression_min (int first, int second)
{
  return GNU_MIN (first, second);
}

int
gnu_statement_expression_once (int *value)
{
  return ({
    int original = (*value)++;

    original + 1;
  });
}

int
gnu_statement_expression_mixed (int value)
{
  return ({
    value++;
    int doubled = value * 2;

    doubled;
  });
}

int
gnu_statement_expression_nested (int value)
{
  return ({
    int inner = ({ value + 1; });

    inner * 2;
  });
}

gnu_bool
gnu_statement_expression_boolean (int value)
{
  return ({
    gnu_bool result = value != 0;

    result;
  });
}

void
gnu_statement_expression_void (int *value)
{
  (void) ({
    (*value)++;
    (void) 0;
  });
}

void
gnu_statement_expression_empty (void)
{
  ({ ; });
}
