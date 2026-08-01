__sfr __at (0x99) SBUF;

static volatile int operand_a = 17;
static volatile int operand_b = 25;
static int evaluation_count;
static int pointer_count;
static int evaluated_result;

static int
evaluated_operand (volatile int *operand)
{
  evaluation_count++;
  return *operand;
}

static int *
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
  signed char signed_char;
  unsigned long long unsigned_long_long;
  _Bool overflow;

  overflow = __builtin_add_overflow (32767, 1, &signed_int);
  if (!overflow)
    failure = 1;
  if (signed_int != -32768)
    failure = 14;
  if (!__builtin_add_overflow (-32768, -1, &signed_int) ||
      signed_int != 32767)
    failure = 2;
  if (!__builtin_add_overflow (65535u, 1u, &unsigned_int) ||
      unsigned_int != 0u)
    failure = 3;
  if (__builtin_add_overflow (65535u, (signed char)-1,
                              &unsigned_int) ||
      unsigned_int != 65534u)
    failure = 4;
  if (!__builtin_add_overflow (127, 1, &signed_char) ||
      signed_char != -128)
    failure = 5;

  if (!__builtin_sub_overflow (-32768, 1, &signed_int) ||
      signed_int != 32767)
    failure = 6;
  if (!__builtin_sub_overflow (0u, 1u, &unsigned_int) ||
      unsigned_int != 65535u)
    failure = 7;
  if (__builtin_sub_overflow (100, 42, &signed_int) || signed_int != 58)
    failure = 8;

  if (!__builtin_mul_overflow (300, 200, &signed_int) ||
      signed_int != -5536)
    failure = 9;
  if (!__builtin_mul_overflow (-128, 2, &signed_char) ||
      signed_char != 0)
    failure = 10;
  if (!__builtin_mul_overflow (0xffffffffffffffffull, 2ull,
                               &unsigned_long_long) ||
      unsigned_long_long != 0xfffffffffffffffeull)
    failure = 11;
  if (__builtin_mul_overflow (-13, 7, &signed_int) || signed_int != -91)
    failure = 12;
  if (__builtin_add_overflow ((_Bool)1, (_Bool)1, &signed_int) ||
      signed_int != 2)
    failure = 15;

  if (__builtin_add_overflow (
        evaluated_operand (&operand_a),
        evaluated_operand (&operand_b), evaluated_pointer ()) ||
      evaluated_result != 42 || evaluation_count != 2 ||
      pointer_count != 1)
    failure = 13;

  if (__builtin_add_overflow (17, 25, &signed_int) ||
      signed_int != 42)
    failure = 16;

  print_result (failure);
  for (;;)
    {
    }
}
