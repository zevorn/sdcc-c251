__sfr __at (0x99) SBUF;

static volatile unsigned int operand_a = 17U;
static volatile unsigned int operand_b = 25U;
static unsigned char evaluation_count;
static unsigned char pointer_count;
static unsigned int evaluated_result;

static unsigned int
evaluated_operand (volatile unsigned int *operand)
{
  evaluation_count++;
  return *operand;
}

static unsigned int *
evaluated_pointer (void)
{
  pointer_count++;
  return &evaluated_result;
}

static void
print_result (unsigned char failure)
{
  const char *text = failure ? "FAIL:" : "PASS\n";

  while (*text)
    SBUF = *text++;
  if (failure)
    {
      const char *hex = "0123456789abcdef";

      SBUF = hex[failure >> 4];
      SBUF = hex[failure & 0x0f];
      SBUF = '\n';
    }
}

void
main (void)
{
  unsigned char failure = 0;
  int signed_int;
  unsigned int unsigned_int;
  long signed_long;
  unsigned long unsigned_long;
  long long signed_long_long;
  unsigned long long unsigned_long_long;

  if (!__builtin_sadd_overflow (32767, 1, &signed_int) ||
      signed_int != -32768)
    failure = 1;
  if (!__builtin_saddl_overflow (2147483647L, 1L, &signed_long) ||
      signed_long != (-2147483647L - 1L))
    failure = 2;
  if (!__builtin_saddll_overflow (9223372036854775807LL, 1LL,
                                  &signed_long_long) ||
      signed_long_long != (-9223372036854775807LL - 1LL))
    failure = 3;
  if (!__builtin_uadd_overflow (65535U, 1U, &unsigned_int) ||
      unsigned_int != 0U)
    failure = 4;
  if (!__builtin_uaddl_overflow (4294967295UL, 1UL, &unsigned_long) ||
      unsigned_long != 0UL)
    failure = 5;
  if (!__builtin_uaddll_overflow (18446744073709551615ULL, 1ULL,
                                  &unsigned_long_long) ||
      unsigned_long_long != 0ULL)
    failure = 6;

  if (!__builtin_ssub_overflow (-32768, 1, &signed_int) ||
      signed_int != 32767)
    failure = 7;
  if (!__builtin_ssubl_overflow ((-2147483647L - 1L), 1L,
                                 &signed_long) ||
      signed_long != 2147483647L)
    failure = 8;
  if (!__builtin_ssubll_overflow ((-9223372036854775807LL - 1LL),
                                  1LL, &signed_long_long) ||
      signed_long_long != 9223372036854775807LL)
    failure = 9;
  if (!__builtin_usub_overflow (0U, 1U, &unsigned_int) ||
      unsigned_int != 65535U)
    failure = 10;
  if (!__builtin_usubl_overflow (0UL, 1UL, &unsigned_long) ||
      unsigned_long != 4294967295UL)
    failure = 11;
  if (!__builtin_usubll_overflow (0ULL, 1ULL, &unsigned_long_long) ||
      unsigned_long_long != 18446744073709551615ULL)
    failure = 12;

  if (!__builtin_smul_overflow (300, 200, &signed_int) ||
      signed_int != -5536)
    failure = 13;
  if (!__builtin_smull_overflow (50000L, 100000L, &signed_long) ||
      signed_long != 705032704L)
    failure = 14;
  if (!__builtin_smulll_overflow (9223372036854775807LL, 2LL,
                                  &signed_long_long) ||
      signed_long_long != -2LL)
    failure = 15;
  if (!__builtin_umul_overflow (300U, 300U, &unsigned_int) ||
      unsigned_int != 24464U)
    failure = 16;
  if (!__builtin_umull_overflow (4294967295UL, 2UL, &unsigned_long) ||
      unsigned_long != 4294967294UL)
    failure = 17;
  if (!__builtin_umulll_overflow (18446744073709551615ULL, 2ULL,
                                  &unsigned_long_long) ||
      unsigned_long_long != 18446744073709551614ULL)
    failure = 18;

  if (!__builtin_uadd_overflow (-1, 1, &unsigned_int) ||
      unsigned_int != 0U)
    failure = 19;
  if (__builtin_smulll_overflow (-13, 7, &signed_long_long) ||
      signed_long_long != -91LL)
    failure = 20;
  if (__builtin_uadd_overflow (
        evaluated_operand (&operand_a), evaluated_operand (&operand_b),
        evaluated_pointer ()) || evaluated_result != 42U ||
      evaluation_count != 2 || pointer_count != 1)
    failure = 21;

  print_result (failure);
  for (;;)
    {
    }
}
