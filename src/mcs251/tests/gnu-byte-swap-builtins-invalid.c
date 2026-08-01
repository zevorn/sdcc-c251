#if GNU_BYTE_SWAP_INVALID_CASE == 1
unsigned int
gnu_bswap16_too_few (void)
{
  return __builtin_bswap16 ();
}
#elif GNU_BYTE_SWAP_INVALID_CASE == 2
unsigned long
gnu_bswap32_too_many (void)
{
  return __builtin_bswap32 (1ul, 2ul);
}
#elif GNU_BYTE_SWAP_INVALID_CASE == 3
unsigned long long
gnu_bswap64_structure (void)
{
  struct gnu_bswap_structure
  {
    int member;
  } value = {0};

  return __builtin_bswap64 (value);
}
#endif
