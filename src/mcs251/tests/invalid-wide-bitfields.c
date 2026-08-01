struct invalid_wide_bitfields
{
  unsigned long too_wide_long : 33;
  unsigned long long too_wide_long_long : 65;
};
