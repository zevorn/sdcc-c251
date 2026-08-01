extern int unavailable_inline_dependency (int);

static inline int
unused_inline_wrapper (int value)
{
  return unavailable_inline_dependency (value);
}

static inline int
called_inline_wrapper (int value)
{
  return value + 1;
}

static inline int
addressed_inline_wrapper (int value)
{
  return value + 2;
}

int
call_inline_wrapper (int value)
{
  return called_inline_wrapper (value);
}

int (*
get_inline_wrapper_address (void)) (int)
{
  return addressed_inline_wrapper;
}
