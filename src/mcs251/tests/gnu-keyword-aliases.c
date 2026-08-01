typedef __typeof__ (1) gnu_typeof_alias;
typedef __typeof (1) gnu_typeof_legacy_alias;

static __inline__ __signed__ int
gnu_keyword_alias_probe (__const__ int * __restrict__ value)
{
  __volatile__ int copy = *value;

  return copy + __alignof__ (gnu_typeof_alias);
}

static __inline __signed int
gnu_keyword_legacy_alias_probe (__const int * __restrict value)
{
  __volatile int copy = *value;

  return copy + __alignof (gnu_typeof_legacy_alias);
}

int
gnu_keyword_alias_call (int value)
{
  return gnu_keyword_alias_probe (&value) +
    gnu_keyword_legacy_alias_probe (&value);
}
